/**
 * @file Watchdog.cpp
 * @brief Implementation of thread-safe watchdog timer management
 */

#include "Watchdog.h"
#include <algorithm>

bool Watchdog::init(uint32_t timeoutSeconds, bool panicOnTimeout) noexcept {
    if (initialized_) {
        WDOG_LOG_W("Watchdog already initialized");
        return true;
    }
    
    if (timeoutSeconds == 0 || timeoutSeconds > 3600) {  // Max 1 hour
        WDOG_LOG_E("Invalid timeout: %lu seconds", timeoutSeconds);
        return false;
    }
    
    timeoutMs_ = timeoutSeconds * 1000;
    panicOnTimeout_ = panicOnTimeout;
    
    esp_err_t err = initWatchdogESPIDF();
    
    if (err == ESP_OK) {
        initialized_ = true;
        WDOG_LOG_I("Watchdog initialized with %lu second timeout", timeoutSeconds);
        return true;
    } else if (err == ESP_ERR_INVALID_STATE) {
        // Already initialized by someone else
        initialized_ = true;
        WDOG_LOG_D("Watchdog was already initialized by another component");
        return true;
    } else {
        WDOG_LOG_E("Failed to initialize watchdog: 0x%x", err);
        return false;
    }
}

bool Watchdog::deinit() noexcept {
    if (!initialized_) {
        return true;
    }
    
    // Unregister all tasks
    if (xSemaphoreTake(taskListMutex_, portMAX_DELAY) == pdTRUE) {
        for (auto& task : registeredTasks_) {
            esp_task_wdt_delete(task.handle);
        }
        registeredTasks_.clear();
        xSemaphoreGive(taskListMutex_);
    }
    
    // Note: ESP-IDF doesn't provide a way to fully deinit the TWDT
    // We can only remove all tasks from it
    initialized_ = false;
    WDOG_LOG_I("Watchdog deinitialized");
    return true;
}

bool Watchdog::registerCurrentTask(const char* taskName, bool isCritical, uint32_t feedIntervalMs) noexcept {
    if (!initialized_) {
        WDOG_LOG_E("Watchdog not initialized");
        return false;
    }
    
    TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
    if (!currentTask) {
        WDOG_LOG_E("Failed to get current task handle");
        return false;
    }
    
    // Check if task is already registered with ESP-IDF watchdog
    esp_err_t status = esp_task_wdt_status(currentTask);
    if (status == ESP_OK) {
        // Already registered with ESP-IDF watchdog
        WDOG_LOG_D("Task %s already registered with ESP-IDF watchdog", taskName);
    } else if (status == ESP_ERR_NOT_FOUND) {
        // Not registered, add it
        esp_err_t err = esp_task_wdt_add(currentTask);
        if (err != ESP_OK) {
            WDOG_LOG_E("Failed to add task %s to watchdog: 0x%x", taskName, err);
            return false;
        }
        WDOG_LOG_D("Task %s added to ESP-IDF watchdog", taskName);
    } else {
        WDOG_LOG_E("Failed to check watchdog status for task %s: 0x%x", taskName, status);
        return false;
    }
    
    // Add to our internal tracking
    if (xSemaphoreTake(taskListMutex_, portMAX_DELAY) == pdTRUE) {
        // Check if already registered
        TaskInfo* existing = findTaskByHandle(currentTask);
        if (existing) {
            WDOG_LOG_W("Task %s already registered", existing->name);
            xSemaphoreGive(taskListMutex_);
            return true;
        }
        
        // Add new task
        TaskInfo info;
        info.handle = currentTask;
        strncpy(info.name, taskName, MAX_TASK_NAME_LEN - 1);
        info.lastFeedTime = xTaskGetTickCount();
        info.feedIntervalMs = (feedIntervalMs > 0) ? feedIntervalMs : (timeoutMs_ / 5);
        info.isCritical = isCritical;
        
        registeredTasks_.push_back(info);
        xSemaphoreGive(taskListMutex_);
        
        // Immediately feed to prevent early timeout
        esp_task_wdt_reset();
        
        WDOG_LOG_I("Task %s registered (critical=%d, interval=%lums)", 
                 taskName, isCritical, info.feedIntervalMs);
        return true;
    }
    
    return false;
}

bool Watchdog::unregisterCurrentTask() noexcept {
    TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
    if (!currentTask) {
        return false;
    }
    
    return unregisterTaskByHandle(currentTask);
}

bool Watchdog::unregisterTaskByHandle(TaskHandle_t taskHandle, const char* taskName) noexcept {
    if (!taskHandle) {
        WDOG_LOG_E("Invalid task handle");
        return false;
    }
    
    // Remove from ESP-IDF watchdog
    esp_err_t err = esp_task_wdt_delete(taskHandle);
    if (err != ESP_OK && err != ESP_ERR_NOT_FOUND) {
        WDOG_LOG_E("Failed to remove task from watchdog: 0x%x", err);
        return false;
    }
    
    // Remove from internal tracking
    if (xSemaphoreTake(taskListMutex_, portMAX_DELAY) == pdTRUE) {
        auto it = std::remove_if(registeredTasks_.begin(), registeredTasks_.end(),
            [taskHandle](const TaskInfo& info) {
                return info.handle == taskHandle;
            });
        
        if (it != registeredTasks_.end()) {
            // Use provided name or lookup name for logging
            const char* logName = taskName ? taskName : it->name;
            WDOG_LOG_I("Task %s unregistered", logName);
            registeredTasks_.erase(it, registeredTasks_.end());
        } else if (taskName) {
            WDOG_LOG_W("Task %s not found in registered list", taskName);
        }
        
        xSemaphoreGive(taskListMutex_);
    }
    
    return true;
}

bool Watchdog::feed() noexcept {
    TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
    if (!currentTask) {
        return false;
    }

    // Update internal tracking if task is registered with us
    if (xSemaphoreTake(taskListMutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
        TaskInfo* info = findTaskByHandle(currentTask);
        if (info) {
            info->lastFeedTime = xTaskGetTickCount();
            info->missedFeeds = 0;
        }
        xSemaphoreGive(taskListMutex_);
    }

    // Only call esp_task_wdt_reset() if task is registered with hardware watchdog
    // This avoids the noisy ESP-IDF error log: "esp_task_wdt_reset(705): task not found"
    // NOTE: We intentionally do NOT auto-register tasks here.
    // Tasks must explicitly call registerCurrentTask() to opt-in to watchdog monitoring.
    esp_err_t status = esp_task_wdt_status(currentTask);
    if (status == ESP_OK) {
        // Task is registered, feed the watchdog
        esp_task_wdt_reset();
    }
    // If not registered (ESP_ERR_NOT_FOUND), silently succeed - this is expected behavior
    return true;
}

size_t Watchdog::getRegisteredTaskCount() const noexcept {
    size_t count = 0;
    if (xSemaphoreTake(taskListMutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
        count = registeredTasks_.size();
        xSemaphoreGive(taskListMutex_);
    }
    return count;
}

bool Watchdog::getTaskInfo(const char* taskName, TaskInfo& info) const {
    if (!taskName) return false;
    
    if (xSemaphoreTake(taskListMutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
        for (const auto& task : registeredTasks_) {
            if (strncmp(task.name, taskName, MAX_TASK_NAME_LEN) == 0) {
                info = task;
                xSemaphoreGive(taskListMutex_);
                return true;
            }
        }
        xSemaphoreGive(taskListMutex_);
    }
    return false;
}

size_t Watchdog::checkHealth() noexcept {
    size_t unhealthyCount = 0;
    TickType_t now = xTaskGetTickCount();
    
    if (xSemaphoreTake(taskListMutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
        for (auto& task : registeredTasks_) {
            TickType_t timeSinceLastFeed = now - task.lastFeedTime;
            uint32_t timeSinceLastFeedMs = timeSinceLastFeed * portTICK_PERIOD_MS;
            
            if (timeSinceLastFeedMs > task.feedIntervalMs * 2) {
                task.missedFeeds++;
                unhealthyCount++;
                WDOG_LOG_W("Task %s hasn't fed watchdog for %lums (expected %lums)", 
                         task.name, timeSinceLastFeedMs, task.feedIntervalMs);
            }
        }
        xSemaphoreGive(taskListMutex_);
    }
    
    return unhealthyCount;
}

Watchdog::TaskInfo* Watchdog::findTaskByHandle(TaskHandle_t handle) {
    for (auto& task : registeredTasks_) {
        if (task.handle == handle) {
            return &task;
        }
    }
    return nullptr;
}

bool Watchdog::updateFeedTime(TaskHandle_t handle) {
    TaskInfo* info = findTaskByHandle(handle);
    if (info) {
        info->lastFeedTime = xTaskGetTickCount();
        info->missedFeeds = 0;
        return true;
    }
    return false;
}

esp_err_t Watchdog::initWatchdogESPIDF() {
    esp_err_t err;
    
    #if ESP_IDF_VERSION_MAJOR >= 5
    // New API for ESP-IDF v5.x
    esp_task_wdt_config_t wdtConfig = {
        .timeout_ms = timeoutMs_,
        .idle_core_mask = 0,  // Don't watch idle tasks
        .trigger_panic = panicOnTimeout_
    };
    err = esp_task_wdt_init(&wdtConfig);
    #else
    // Legacy API for ESP-IDF v4.x and earlier
    err = esp_task_wdt_init(timeoutMs_ / 1000, panicOnTimeout_);
    #endif
    
    return err;
}