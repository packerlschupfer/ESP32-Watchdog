/**
 * @file Watchdog.h
 * @brief Thread-safe watchdog timer management for ESP32 FreeRTOS tasks
 * 
 * This class provides a robust abstraction over ESP-IDF's task watchdog timer (TWDT) API,
 * handling version differences and ensuring proper thread context for operations.
 * 
 * @note Tasks MUST register and feed the watchdog from their own execution context
 * @author TaskManager Contributors
 * @date 2025-06-13
 */

#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <esp_task_wdt.h>
#include <esp_err.h>
#include <atomic>
#include <vector>
#include <cstring>

// Include logging configuration (C++11 compatible)
#include "WatchdogLog.h"
#include "IWatchdog.h"

/**
 * @class Watchdog
 * @brief Singleton manager for ESP32 task watchdog timer with thread safety
 * 
 * This class implements a singleton pattern because ESP-IDF's task watchdog
 * timer (TWDT) is a global resource. Multiple instances would conflict as they
 * would all try to manage the same underlying ESP-IDF watchdog.
 * 
 * Features:
 * - Singleton pattern ensures only one instance manages ESP-IDF TWDT
 * - Automatic ESP-IDF version detection and API adaptation
 * - Thread-safe task registration and feeding
 * - Per-task timeout tracking
 * - Graceful error handling
 * - Support for both critical and non-critical tasks
 * 
 * Usage example:
 * @code
 * // Option 1: Using instance reference
 * Watchdog& watchdog = Watchdog::getInstance();
 * watchdog.init(30, true);  // 30 second timeout, panic on trigger
 * 
 * void myTask(void* params) {
 *     watchdog.registerCurrentTask("MyTask");
 *     while (true) {
 *         // Do work...
 *         watchdog.feed();
 *         vTaskDelay(pdMS_TO_TICKS(1000));
 *     }
 * }
 * 
 * // Option 2: Using static convenience methods
 * void setup() {
 *     Watchdog::quickInit(30, true);
 * }
 * 
 * void myTask(void* params) {
 *     Watchdog::quickRegister("MyTask");
 *     while (true) {
 *         // Do work...
 *         Watchdog::quickFeed();
 *         vTaskDelay(pdMS_TO_TICKS(1000));
 *     }
 * }
 * @endcode
 */
class Watchdog : public IWatchdog {
private:
    /**
     * @brief Private constructor for singleton pattern
     */
    Watchdog() : initialized_(false), timeoutMs_(DEFAULT_TIMEOUT_MS), 
                 panicOnTimeout_(true), taskListMutex_(nullptr) {
        taskListMutex_ = xSemaphoreCreateMutex();
        configASSERT(taskListMutex_ != nullptr);
    }
    
    /**
     * @brief Private destructor - watchdog singleton should never be destroyed
     */
    ~Watchdog() {
        if (initialized_) {
            deinit();
        }
        if (taskListMutex_) {
            vSemaphoreDelete(taskListMutex_);
        }
    }
    
    // Delete copy and move constructors/operators
    Watchdog(const Watchdog&) = delete;
    Watchdog& operator=(const Watchdog&) = delete;
    Watchdog(Watchdog&&) = delete;
    Watchdog& operator=(Watchdog&&) = delete;

public:
    static constexpr const char* TAG = "Watchdog";
    static constexpr size_t MAX_TASK_NAME_LEN = configMAX_TASK_NAME_LEN;
    static constexpr uint32_t DEFAULT_TIMEOUT_MS = 30000;  // 30 seconds
    static constexpr uint32_t MIN_TIMEOUT_MS = 1000;       // 1 second
    
    /**
     * @brief Task registration info for internal tracking
     */
    struct TaskInfo {
        TaskHandle_t handle;
        char name[MAX_TASK_NAME_LEN];
        TickType_t lastFeedTime;
        uint32_t feedIntervalMs;
        std::atomic<uint32_t> missedFeeds{0};
        bool isCritical;
        
        TaskInfo() : handle(nullptr), lastFeedTime(0), feedIntervalMs(0), isCritical(false) {
            memset(name, 0, MAX_TASK_NAME_LEN);
        }
        
        // Copy constructor
        TaskInfo(const TaskInfo& other) 
            : handle(other.handle), 
              lastFeedTime(other.lastFeedTime),
              feedIntervalMs(other.feedIntervalMs),
              missedFeeds(other.missedFeeds.load()),
              isCritical(other.isCritical) {
            memcpy(name, other.name, MAX_TASK_NAME_LEN);
        }
        
        // Copy assignment
        TaskInfo& operator=(const TaskInfo& other) {
            if (this != &other) {
                handle = other.handle;
                memcpy(name, other.name, MAX_TASK_NAME_LEN);
                lastFeedTime = other.lastFeedTime;
                feedIntervalMs = other.feedIntervalMs;
                missedFeeds = other.missedFeeds.load();
                isCritical = other.isCritical;
            }
            return *this;
        }
    };
    
    /**
     * @brief Get the singleton instance of Watchdog
     * @return Reference to the singleton Watchdog instance
     * @note Thread-safe initialization guaranteed by C++11
     */
    static Watchdog& getInstance() {
        static Watchdog instance;
        return instance;
    }
    
    // ============== IWatchdog Interface Implementation ==============

    /**
     * @brief Initialize the watchdog timer
     * @param timeoutSeconds Timeout in seconds before watchdog triggers
     * @param panicOnTimeout If true, system will panic/reset on timeout
     * @return true if initialization successful
     */
    bool init(uint32_t timeoutSeconds = 30, bool panicOnTimeout = true) noexcept override;

    /**
     * @brief Deinitialize the watchdog timer
     * @return true if deinitialization successful
     */
    bool deinit() noexcept override;

    /**
     * @brief Register current task with watchdog (MUST be called from task context)
     * @param taskName Name for identification
     * @param isCritical If true, timeout will trigger panic
     * @param feedIntervalMs Expected feed interval (0 = auto-calculate)
     * @return true if registration successful
     */
    bool registerCurrentTask(const char* taskName, bool isCritical = true,
                           uint32_t feedIntervalMs = 0) noexcept override;

    /**
     * @brief Unregister current task from watchdog
     * @return true if unregistration successful
     */
    bool unregisterCurrentTask() noexcept override;

    /**
     * @brief Unregister a task from watchdog by its handle
     * @param taskHandle Handle of the task to unregister
     * @param taskName Optional name for logging (nullptr = lookup name)
     * @return true if unregistration successful
     * @note Can be called from any task context
     */
    bool unregisterTaskByHandle(TaskHandle_t taskHandle, const char* taskName = nullptr) noexcept override;

    /**
     * @brief Feed the watchdog for current task
     * @return true if feed successful
     * @note MUST be called from registered task context
     */
    bool feed() noexcept override;

    /**
     * @brief Check if watchdog is initialized
     * @return true if initialized
     */
    bool isInitialized() const noexcept override { return initialized_; }

    /**
     * @brief Get current timeout in milliseconds
     * @return Timeout in milliseconds
     */
    uint32_t getTimeoutMs() const noexcept override { return timeoutMs_; }

    /**
     * @brief Get number of registered tasks
     * @return Number of tasks registered with watchdog
     */
    size_t getRegisteredTaskCount() const noexcept override;

    /**
     * @brief Check health of all registered tasks
     * @return Number of tasks that haven't fed watchdog recently
     */
    size_t checkHealth() noexcept override;

    // ============== Non-Interface Methods ==============

    /**
     * @brief Get task info by name
     * @param taskName Name of task to find
     * @param info Output parameter for task info
     * @return true if task found
     */
    bool getTaskInfo(const char* taskName, TaskInfo& info) const;
    
    // ============== Static Convenience Methods ==============
    
    /**
     * @brief Quick initialization with default or custom settings
     * @param timeoutSeconds Timeout in seconds (default: 30)
     * @param panicOnTimeout Panic on timeout (default: true)
     * @return true if initialization successful
     */
    static bool quickInit(uint32_t timeoutSeconds = 30, bool panicOnTimeout = true) {
        return getInstance().init(timeoutSeconds, panicOnTimeout);
    }
    
    /**
     * @brief Quick feed from current task
     * @return true if feed successful
     */
    static bool quickFeed() {
        return getInstance().feed();
    }
    
    /**
     * @brief Quick register current task
     * @param taskName Name for identification
     * @param isCritical If true, timeout will trigger panic (default: true)
     * @param feedIntervalMs Expected feed interval (0 = auto-calculate)
     * @return true if registration successful
     */
    static bool quickRegister(const char* taskName, bool isCritical = true, 
                             uint32_t feedIntervalMs = 0) {
        return getInstance().registerCurrentTask(taskName, isCritical, feedIntervalMs);
    }
    
    /**
     * @brief Check if watchdog is globally initialized
     * @return true if the singleton instance is initialized
     */
    static bool isGloballyInitialized() {
        return getInstance().isInitialized();
    }
    
    /**
     * @brief Get global count of registered tasks
     * @return Number of tasks registered with the singleton
     */
    static size_t getGlobalTaskCount() {
        return getInstance().getRegisteredTaskCount();
    }
    
    /**
     * @brief Quick health check of all tasks
     * @return Number of unhealthy tasks
     */
    static size_t quickCheckHealth() {
        return getInstance().checkHealth();
    }

private:
    std::atomic<bool> initialized_;
    uint32_t timeoutMs_;
    bool panicOnTimeout_;
    SemaphoreHandle_t taskListMutex_;
    std::vector<TaskInfo> registeredTasks_;
    
    /**
     * @brief Find task info by handle
     * @param handle Task handle to search for
     * @return Pointer to TaskInfo or nullptr if not found
     */
    TaskInfo* findTaskByHandle(TaskHandle_t handle);
    
    /**
     * @brief Update feed time for task
     * @param handle Task handle
     * @return true if task found and updated
     */
    bool updateFeedTime(TaskHandle_t handle);
    
    /**
     * @brief ESP-IDF version-specific initialization
     */
    esp_err_t initWatchdogESPIDF();
};

#endif // WATCHDOG_H