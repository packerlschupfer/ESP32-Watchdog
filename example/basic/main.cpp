/**
 * @file main.cpp
 * @brief Basic example of Watchdog library usage
 * 
 * This example demonstrates:
 * - Proper task registration from task context
 * - Critical vs non-critical tasks
 * - Health monitoring
 * - Error handling
 * - Optional custom Logger integration via LogInterface
 * 
 * To use with custom Logger, define USE_CUSTOM_LOGGER in build flags
 */

// Optional: Enable custom Logger instead of ESP-IDF logging
// #define USE_CUSTOM_LOGGER

#ifdef USE_CUSTOM_LOGGER
    #include <Logger.h>
    #include <LogInterfaceImpl.h>  // Include ONCE in main.cpp
#endif

#include <Arduino.h>
#include <Watchdog.h>

// Get the singleton watchdog instance
Watchdog& watchdog = Watchdog::getInstance();

// LED task - blinks LED and feeds watchdog regularly
void ledTask(void* pvParameters) {
    const int ledPin = 2;
    pinMode(ledPin, OUTPUT);
    
    // Register this task as critical (will panic on timeout)
    if (!watchdog.registerCurrentTask("LED", true, 1000)) {
        Serial.println("LED: Failed to register with watchdog!");
        vTaskDelete(NULL);
        return;
    }
    
    Serial.println("LED: Task started and registered");
    
    while (true) {
        digitalWrite(ledPin, HIGH);
        vTaskDelay(pdMS_TO_TICKS(250));
        
        digitalWrite(ledPin, LOW);
        vTaskDelay(pdMS_TO_TICKS(250));
        
        // Feed watchdog every 500ms
        if (!watchdog.feed()) {
            Serial.println("LED: Failed to feed watchdog!");
        }
    }
}

// Sensor task - simulates sensor reading
void sensorTask(void* pvParameters) {
    // Register as non-critical (won't panic)
    if (!watchdog.registerCurrentTask("Sensor", false, 5000)) {
        Serial.println("Sensor: Failed to register with watchdog!");
        vTaskDelete(NULL);
        return;
    }
    
    Serial.println("Sensor: Task started and registered");
    
    int reading = 0;
    while (true) {
        // Simulate sensor reading
        reading = analogRead(34);  // GPIO34
        Serial.printf("Sensor: Reading = %d\n", reading);
        
        // Feed watchdog before delay
        watchdog.feed();
        
        // Delay 3 seconds between readings
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

// Monitor task - checks system health
void monitorTask(void* pvParameters) {
    // Wait for system to stabilize
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    Serial.println("Monitor: Starting health checks");
    
    while (true) {
        // Check watchdog health
        size_t unhealthyCount = watchdog.checkHealth();
        size_t totalTasks = watchdog.getRegisteredTaskCount();
        
        Serial.printf("Monitor: Watchdog status - %zu/%zu tasks healthy\n", 
                      totalTasks - unhealthyCount, totalTasks);
        
        // Check specific task
        Watchdog::TaskInfo info;
        if (watchdog.getTaskInfo("LED", info)) {
            Serial.printf("Monitor: LED task - last fed %lu ms ago, missed feeds: %lu\n",
                          (xTaskGetTickCount() - info.lastFeedTime) * portTICK_PERIOD_MS,
                          info.missedFeeds.load());
        }
        
        // Report memory usage
        Serial.printf("Monitor: Free heap = %d bytes\n", ESP.getFreeHeap());
        
        vTaskDelay(pdMS_TO_TICKS(10000));  // Check every 10 seconds
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    
    Serial.println("\n\n=== Watchdog Library Example ===");
    Serial.printf("ESP32 Chip Model: %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
    Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
    
    #ifdef USE_CUSTOM_LOGGER
    // Initialize custom Logger if enabled
    Serial.println("Initializing custom Logger...");
    Logger& logger = Logger::getInstance();
    logger.init(1024);
    logger.setLogLevel(ESP_LOG_DEBUG);
    logger.enableLogging(true);
    Serial.println("Custom Logger initialized - Watchdog will use Logger");
    #else
    Serial.println("Using ESP-IDF logging (default)");
    #endif
    
    // Initialize watchdog with 30 second timeout
    if (!watchdog.init(30, true)) {
        Serial.println("ERROR: Failed to initialize watchdog!");
        Serial.println("System will continue without watchdog protection");
    } else {
        Serial.println("Watchdog initialized successfully (30s timeout)");
    }
    
    // Create tasks
    if (xTaskCreate(ledTask, "LED", 4096, NULL, 2, NULL) != pdPASS) {
        Serial.println("ERROR: Failed to create LED task!");
    }
    
    if (xTaskCreate(sensorTask, "Sensor", 4096, NULL, 1, NULL) != pdPASS) {
        Serial.println("ERROR: Failed to create Sensor task!");
    }
    
    if (xTaskCreate(monitorTask, "Monitor", 4096, NULL, 1, NULL) != pdPASS) {
        Serial.println("ERROR: Failed to create Monitor task!");
    }
    
    Serial.println("Setup complete - all tasks created");
}

void loop() {
    // Main loop doesn't use watchdog
    // Just keep it simple
    vTaskDelay(pdMS_TO_TICKS(1000));
}