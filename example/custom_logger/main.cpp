/**
 * @file main.cpp
 * @brief Example of using Watchdog library with custom Logger
 * 
 * This example demonstrates how to use the Watchdog library with
 * the custom Logger library for enhanced logging capabilities.
 */

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Include Logger and implementation (ONCE in main.cpp)
#include <Logger.h>
#include <LogInterfaceImpl.h>

// Now include Watchdog - it will use custom Logger
#include <Watchdog.h>

// Get the singleton watchdog instance
Watchdog& watchdog = Watchdog::getInstance();

// Task that feeds regularly
void normalTask(void* parameter) {
    const char* taskName = "NormalTask";
    
    // Register this task with the watchdog
    // Must be called from within the task!
    if (!watchdog.registerCurrentTask(taskName, true, 2000)) {
        Serial.printf("[%s] Failed to register with watchdog\n", taskName);
        vTaskDelete(NULL);
        return;
    }
    
    Serial.printf("[%s] Registered with watchdog\n", taskName);
    
    uint32_t count = 0;
    while (true) {
        // Simulate work
        Serial.printf("[%s] Working... count=%lu\n", taskName, count++);
        
        // Feed the watchdog
        if (!watchdog.feed()) {
            Serial.printf("[%s] Failed to feed watchdog!\n", taskName);
        }
        
        // Delay for 1 second
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Task that intentionally stops feeding
void problematicTask(void* parameter) {
    const char* taskName = "ProblematicTask";
    
    // Register as non-critical (won't cause panic)
    if (!watchdog.registerCurrentTask(taskName, false, 3000)) {
        Serial.printf("[%s] Failed to register with watchdog\n", taskName);
        vTaskDelete(NULL);
        return;
    }
    
    Serial.printf("[%s] Registered with watchdog (non-critical)\n", taskName);
    
    // Feed for 10 seconds then stop
    for (int i = 0; i < 10; i++) {
        Serial.printf("[%s] Feeding... %d/10\n", taskName, i + 1);
        watchdog.feed();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    // Simulate a hung task
    Serial.printf("[%s] Simulating hang - stopping feeds\n", taskName);
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Health monitor task
void monitorTask(void* parameter) {
    // Wait for system to stabilize
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    while (true) {
        size_t unhealthyCount = watchdog.checkHealth();
        
        if (unhealthyCount > 0) {
            Serial.printf("[Monitor] WARNING: %zu unhealthy task(s) detected!\n", unhealthyCount);
            
            // Get info about the problematic task
            Watchdog::TaskInfo info;
            if (watchdog.getTaskInfo("ProblematicTask", info)) {
                Serial.printf("[Monitor] Task '%s': missed feeds=%lu, last feed=%lu ms ago\n",
                    info.taskName, info.missedFeeds, 
                    (xTaskGetTickCount() - info.lastFeedTime) * portTICK_PERIOD_MS);
            }
        } else {
            Serial.printf("[Monitor] All tasks healthy\n");
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }
    
    Serial.println("\n=== Watchdog with Custom Logger Example ===\n");
    
    // Initialize custom Logger
    Logger& logger = Logger::getInstance();
    logger.init(1024);  // 1KB buffer
    logger.setLogLevel(ESP_LOG_DEBUG);  // Show debug logs
    logger.enableLogging(true);
    
    Serial.println("Custom Logger initialized");
    
    // Initialize watchdog with 30 second timeout
    if (!watchdog.init(30, true)) {
        Serial.println("ERROR: Failed to initialize watchdog!");
        return;
    }
    
    Serial.println("Watchdog initialized with 30s timeout");
    Serial.println("Creating tasks...\n");
    
    // Create tasks
    xTaskCreatePinnedToCore(
        normalTask,
        "NormalTask",
        4096,
        NULL,
        1,
        NULL,
        1
    );
    
    xTaskCreatePinnedToCore(
        problematicTask,
        "ProblematicTask",
        4096,
        NULL,
        1,
        NULL,
        1
    );
    
    xTaskCreatePinnedToCore(
        monitorTask,
        "MonitorTask",
        4096,
        NULL,
        1,
        NULL,
        0
    );
}

void loop() {
    // Print status every 10 seconds
    static uint32_t lastPrint = 0;
    uint32_t now = millis();
    
    if (now - lastPrint >= 10000) {
        lastPrint = now;
        
        size_t registeredCount = watchdog.getRegisteredTaskCount();
        Serial.printf("\n[Main] Watchdog status: %zu task(s) registered\n", registeredCount);
        
        // The watchdog logs will appear through custom Logger with timestamps
        // and any other features configured in the Logger
    }
    
    delay(100);
}