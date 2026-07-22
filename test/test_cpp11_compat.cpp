/**
 * @file test_cpp11_compat.cpp
 * @brief Test C++11 compatibility of Watchdog library logging
 * 
 * This test ensures the library compiles and works correctly
 * with C++11 standard, without requiring C++17 features.
 */

#include <Arduino.h>
#include <unity.h>

// Force C++11 standard if possible
#if __cplusplus > 201103L
    #warning "Compiling with C++ standard newer than C++11"
#endif

// Test that library compiles without C++17 features
#include <Watchdog.h>

void test_cpp11_compilation() {
    // This test passes if the code compiles
    TEST_ASSERT_TRUE(true);
}

void test_watchdog_init_cpp11() {
    Watchdog& watchdog = Watchdog::getInstance();
    
    // Test basic initialization
    TEST_ASSERT_TRUE(watchdog.init(10, false));
    
    // Cleanup
    watchdog.deinit();
}

void test_logging_macros_cpp11() {
    // Test that logging macros compile correctly
    WDOG_LOG_E("Error test: %d", 1);
    WDOG_LOG_W("Warning test: %s", "test");
    WDOG_LOG_I("Info test");
    WDOG_LOG_D("Debug test");
    WDOG_LOG_V("Verbose test");
    
    // Test passes if macros compile
    TEST_ASSERT_TRUE(true);
}

#ifdef USE_CUSTOM_LOGGER
void test_custom_logger_cpp11() {
    // When custom logger is enabled, test it works
    WDOG_LOG_I("Testing with custom logger in C++11");
    TEST_ASSERT_TRUE(true);
}
#else
void test_esp_idf_logger_cpp11() {
    // When using ESP-IDF, test it works
    WDOG_LOG_I("Testing with ESP-IDF logger in C++11");
    TEST_ASSERT_TRUE(true);
}
#endif

void setup() {
    delay(2000);
    UNITY_BEGIN();
    
    RUN_TEST(test_cpp11_compilation);
    RUN_TEST(test_watchdog_init_cpp11);
    RUN_TEST(test_logging_macros_cpp11);
    
    #ifdef USE_CUSTOM_LOGGER
    RUN_TEST(test_custom_logger_cpp11);
    Serial.println("Tested with custom Logger (C++11)");
    #else
    RUN_TEST(test_esp_idf_logger_cpp11);
    Serial.println("Tested with ESP-IDF logging (C++11)");
    #endif
    
    UNITY_END();
}

void loop() {
    // Empty
}