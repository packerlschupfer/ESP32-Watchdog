/**
 * @file main.cpp
 * @brief Example demonstrating C++11 compatibility of Watchdog library
 * 
 * This example shows that the Watchdog library works correctly with
 * C++11 standard and doesn't require any C++17 features.
 */

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Optional: Enable custom logger (comment out to use ESP-IDF)
// #define USE_CUSTOM_LOGGER

#ifdef USE_CUSTOM_LOGGER
    // When using custom logger, include these in main.cpp
    #include <Logger.h>
    #include <LogInterfaceImpl.h>
#endif

// Include Watchdog - it will use appropriate logging based on USE_CUSTOM_LOGGER
#include <Watchdog.h>

// Get the singleton watchdog instance
Watchdog& watchdog = Watchdog::getInstance();

// Simple task that demonstrates watchdog usage
void simpleTask(void* parameter) {
    const char* taskName = static_cast<const char*>(parameter);
    
    // Register with watchdog (C++11 style initialization)
    if (!watchdog.registerCurrentTask(taskName, true, 2000)) {
        Serial.printf("[%s] Failed to register with watchdog\n", taskName);
        vTaskDelete(NULL);
        return;
    }
    
    Serial.printf("[%s] Registered with watchdog\n", taskName);
    
    // C++11 style loop with auto
    for (auto i = 0; i < 100; ++i) {
        // Feed watchdog
        watchdog.feed();
        
        // Log using library macros (works with both loggers)
        if (i % 10 == 0) {
            Serial.printf("[%s] Progress: %d/100\n", taskName, i);
        }
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    
    Serial.printf("[%s] Task completed\n", taskName);
    watchdog.unregisterCurrentTask();
    vTaskDelete(NULL);
}

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    
    Serial.println("\n=== Watchdog C++11 Compatibility Example ===");
    Serial.printf("C++ Standard: %ld\n", __cplusplus);
    
    #ifdef USE_CUSTOM_LOGGER
        Serial.println("Using custom Logger");
        // Initialize Logger (C++11 style)
        auto& logger = Logger::getInstance();
        logger.init(1024);
        logger.setLogLevel(ESP_LOG_DEBUG);
        logger.enableLogging(true);
    #else
        Serial.println("Using ESP-IDF logging");
    #endif
    
    // Initialize watchdog with C++11 style
    if (!watchdog.init(30, true)) {
        Serial.println("ERROR: Failed to initialize watchdog!");
        return;
    }
    
    Serial.println("Watchdog initialized successfully");
    
    // Create tasks using C++11 lambda for variety
    const char* task1Name = "Task1";
    const char* task2Name = "Task2";
    
    xTaskCreate(simpleTask, task1Name, 4096, 
                const_cast<char*>(task1Name), 1, nullptr);
    
    xTaskCreate(simpleTask, task2Name, 4096, 
                const_cast<char*>(task2Name), 1, nullptr);
    
    // Use C++11 range-based for loop example
    const char* messages[] = {
        "System started",
        "Tasks created",
        "Watchdog active"
    };
    
    for (const auto& msg : messages) {
        Serial.println(msg);
    }
}

void loop() {
    // C++11 style timing
    static auto lastReport = millis();
    auto now = millis();
    
    if (now - lastReport >= 10000) {
        lastReport = now;
        
        // Report status using C++11 features
        auto taskCount = watchdog.getRegisteredTaskCount();
        Serial.printf("[Main] Status: %zu task(s) registered\n", taskCount);
        
        // Check health
        auto unhealthyCount = watchdog.checkHealth();
        if (unhealthyCount > 0) {
            Serial.printf("[Main] Warning: %zu unhealthy task(s)\n", unhealthyCount);
        }
    }
    
    delay(100);
}