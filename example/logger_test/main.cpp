/**
 * @file main.cpp
 * @brief Test Watchdog library with LogInterface migration
 * 
 * This test verifies:
 * - Library works without USE_CUSTOM_LOGGER (ESP-IDF logging)
 * - Library works with USE_CUSTOM_LOGGER (custom Logger)
 * - No memory overhead when Logger not used
 */

// Uncomment to test with custom Logger
// #define USE_CUSTOM_LOGGER

#ifdef USE_CUSTOM_LOGGER
    #include <Logger.h>
    #include <LogInterfaceImpl.h>
#endif

#include <Arduino.h>
#include <Watchdog.h>

Watchdog watchdog;

void testTask(void* pvParameters) {
    const char* taskName = (const char*)pvParameters;
    
    // Register with watchdog
    if (!watchdog.registerCurrentTask(taskName, true, 2000)) {
        Serial.printf("%s: Failed to register!\n", taskName);
        vTaskDelete(NULL);
        return;
    }
    
    Serial.printf("%s: Registered successfully\n", taskName);
    
    // Run for 10 seconds feeding watchdog
    for (int i = 0; i < 10; i++) {
        Serial.printf("%s: Working... %d/10\n", taskName, i + 1);
        
        if (!watchdog.feed()) {
            Serial.printf("%s: Failed to feed watchdog!\n", taskName);
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    // Unregister and exit
    watchdog.unregisterCurrentTask();
    Serial.printf("%s: Task completed\n", taskName);
    vTaskDelete(NULL);
}

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    
    Serial.println("\n=== Watchdog LogInterface Test ===");
    
    // Memory before initialization
    uint32_t heapBefore = ESP.getFreeHeap();
    Serial.printf("Free heap before: %u bytes\n", heapBefore);
    
    #ifdef USE_CUSTOM_LOGGER
    Serial.println("\n[Using Custom Logger]");
    Logger& logger = Logger::getInstance();
    logger.init(1024);
    logger.setLogLevel(ESP_LOG_VERBOSE);
    logger.enableLogging(true);
    
    uint32_t heapAfterLogger = ESP.getFreeHeap();
    Serial.printf("Free heap after Logger init: %u bytes\n", heapAfterLogger);
    Serial.printf("Logger memory usage: %u bytes\n", heapBefore - heapAfterLogger);
    #else
    Serial.println("\n[Using ESP-IDF Logging]");
    #endif
    
    // Initialize watchdog
    if (!watchdog.init(30, true)) {
        Serial.println("ERROR: Failed to initialize watchdog!");
        return;
    }
    
    uint32_t heapAfterWatchdog = ESP.getFreeHeap();
    Serial.printf("Free heap after Watchdog init: %u bytes\n", heapAfterWatchdog);
    
    #ifdef USE_CUSTOM_LOGGER
    Serial.printf("Watchdog memory usage: %u bytes\n", heapAfterLogger - heapAfterWatchdog);
    #else
    Serial.printf("Total memory usage (no Logger): %u bytes\n", heapBefore - heapAfterWatchdog);
    #endif
    
    // Create test task
    xTaskCreate(testTask, "TestTask", 4096, (void*)"TestTask", 1, NULL);
    
    Serial.println("\nTest running... Check log output");
}

void loop() {
    static uint32_t lastReport = 0;
    
    if (millis() - lastReport > 5000) {
        lastReport = millis();
        
        size_t taskCount = watchdog.getRegisteredTaskCount();
        Serial.printf("\nStatus: %zu tasks registered\n", taskCount);
        
        size_t unhealthy = watchdog.checkHealth();
        if (unhealthy > 0) {
            Serial.printf("WARNING: %zu unhealthy tasks!\n", unhealthy);
        }
    }
    
    vTaskDelay(pdMS_TO_TICKS(100));
}