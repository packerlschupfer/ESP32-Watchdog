/**
 * @file main.cpp
 * @brief Test different logging configurations for Watchdog library
 * 
 * This example demonstrates:
 * - Default ESP-IDF logging
 * - Custom Logger integration
 * - Debug vs Release logging behavior
 * - Zero overhead in release builds
 */

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Check build configuration
#ifdef USE_CUSTOM_LOGGER
    #include <Logger.h>
    #include <LogInterfaceImpl.h>
    #define LOGGER_TYPE "Custom Logger"
#else
    #define LOGGER_TYPE "ESP-IDF"
#endif

#ifdef WATCHDOG_DEBUG
    #define DEBUG_MODE "Debug Mode"
#else
    #define DEBUG_MODE "Release Mode"
#endif

#include <Watchdog.h>

// Get the singleton watchdog instance
Watchdog& watchdog = Watchdog::getInstance();

// Task to demonstrate all log levels
void logTestTask(void* parameter) {
    const char* taskName = "LogTest";
    
    Serial.println("\n--- Testing Watchdog Logging ---");
    Serial.printf("Configuration: %s + %s\n", LOGGER_TYPE, DEBUG_MODE);
    Serial.println("Expected behavior:");
    Serial.println("- Release mode: Only ERROR, WARN, INFO logs visible");
    Serial.println("- Debug mode: All log levels visible");
    Serial.println("\nStarting log tests...\n");
    
    // Register with watchdog - this will generate INFO log
    Serial.println("1. Registering task (should see INFO log):");
    if (!watchdog.registerCurrentTask(taskName, true, 5000)) {
        vTaskDelete(NULL);
        return;
    }
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Test all log levels by triggering different scenarios
    Serial.println("\n2. Testing error condition (should see ERROR log):");
    // Try to register again - will trigger warning
    watchdog.registerCurrentTask(taskName, true, 5000);
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    Serial.println("\n3. Normal operation (should see INFO logs):");
    // Feed watchdog - generates debug logs
    for (int i = 0; i < 3; i++) {
        watchdog.feed();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    Serial.println("\n4. Health check (may generate WARN logs if unhealthy):");
    size_t unhealthy = watchdog.checkHealth();
    Serial.printf("Unhealthy tasks: %zu\n", unhealthy);
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    Serial.println("\n5. Unregistering task (should see INFO log):");
    watchdog.unregisterCurrentTask();
    
    Serial.println("\n--- Log Test Complete ---");
    Serial.println("Check above output to verify logging behavior matches configuration.");
    
    vTaskDelete(NULL);
}

// Task that intentionally stops feeding to generate warnings
void problematicTask(void* parameter) {
    const char* taskName = "Problem";
    
    // Register as non-critical
    if (!watchdog.registerCurrentTask(taskName, false, 3000)) {
        vTaskDelete(NULL);
        return;
    }
    
    // Feed for a bit then stop
    for (int i = 0; i < 5; i++) {
        watchdog.feed();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    // Stop feeding to trigger warnings
    Serial.println("\n[Problem Task] Simulating hang - stopping feeds");
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    
    Serial.println("\n\n=== Watchdog Logging Test ===");
    Serial.printf("Build Configuration:\n");
    Serial.printf("- Logger Backend: %s\n", LOGGER_TYPE);
    Serial.printf("- Debug Mode: %s\n", DEBUG_MODE);
    Serial.printf("- Free Heap Before: %d bytes\n", ESP.getFreeHeap());
    
    #ifdef USE_CUSTOM_LOGGER
        // Initialize custom logger
        Logger& logger = Logger::getInstance();
        logger.init(1024);
        logger.setLogLevel(ESP_LOG_VERBOSE);  // Allow all levels
        logger.enableLogging(true);
        Serial.println("- Custom Logger initialized");
    #endif
    
    // Initialize watchdog
    if (!watchdog.init(30, true)) {
        Serial.println("ERROR: Failed to initialize watchdog!");
        return;
    }
    
    Serial.printf("- Free Heap After Init: %d bytes\n", ESP.getFreeHeap());
    Serial.println("\nCreating test tasks...\n");
    
    // Create log test task
    xTaskCreate(logTestTask, "LogTest", 4096, NULL, 1, NULL);
    
    // Create problematic task to generate warnings
    xTaskCreate(problematicTask, "Problem", 4096, NULL, 1, NULL);
}

void loop() {
    static uint32_t lastCheck = 0;
    uint32_t now = millis();
    
    // Every 10 seconds, check health to potentially generate logs
    if (now - lastCheck >= 10000) {
        lastCheck = now;
        
        Serial.println("\n[Main] Performing health check...");
        size_t unhealthy = watchdog.checkHealth();
        
        if (unhealthy > 0) {
            Serial.printf("[Main] Found %zu unhealthy task(s)\n", unhealthy);
            
            // Get detailed info
            Watchdog::TaskInfo info;
            if (watchdog.getTaskInfo("Problem", info)) {
                Serial.printf("[Main] Problem task: missed=%lu, last feed=%lu ms ago\n",
                    info.missedFeeds,
                    (xTaskGetTickCount() - info.lastFeedTime) * portTICK_PERIOD_MS);
            }
        }
    }
    
    delay(100);
}