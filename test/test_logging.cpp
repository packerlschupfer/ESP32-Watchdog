/**
 * @file test_logging.cpp
 * @brief Test that Watchdog library works with both ESP-IDF and custom Logger
 */

#include <Arduino.h>
#include <unity.h>

// Test 1: Verify library compiles and works without USE_CUSTOM_LOGGER
#ifndef USE_CUSTOM_LOGGER
#include <Watchdog.h>

void test_esp_idf_logging() {
    Watchdog& watchdog = Watchdog::getInstance();
    
    // Should use ESP-IDF logging
    TEST_ASSERT_TRUE(watchdog.init(10, false));
    
    // Register current task
    TEST_ASSERT_TRUE(watchdog.registerCurrentTask("TestTask", false, 1000));
    
    // Feed should work
    TEST_ASSERT_TRUE(watchdog.feed());
    
    // Cleanup
    watchdog.deinit();
}

#else
// Test 2: Verify library works with USE_CUSTOM_LOGGER defined
#include <Logger.h>
#include <LogInterfaceImpl.h>
#include <Watchdog.h>

void test_custom_logger() {
    // Initialize Logger
    Logger& logger = Logger::getInstance();
    logger.init(512);
    logger.setLogLevel(ESP_LOG_DEBUG);
    
    Watchdog& watchdog = Watchdog::getInstance();
    
    // Should use custom Logger
    TEST_ASSERT_TRUE(watchdog.init(10, false));
    
    // Register current task
    TEST_ASSERT_TRUE(watchdog.registerCurrentTask("TestTask", false, 1000));
    
    // Feed should work
    TEST_ASSERT_TRUE(watchdog.feed());
    
    // Cleanup
    watchdog.deinit();
}
#endif

void setup() {
    delay(2000);
    UNITY_BEGIN();
    
    #ifndef USE_CUSTOM_LOGGER
    RUN_TEST(test_esp_idf_logging);
    Serial.println("Tested with ESP-IDF logging");
    #else
    RUN_TEST(test_custom_logger);
    Serial.println("Tested with custom Logger");
    #endif
    
    UNITY_END();
}

void loop() {
    // Empty
}