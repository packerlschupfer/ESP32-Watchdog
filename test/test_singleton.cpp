/**
 * @file test_singleton.cpp
 * @brief Test singleton behavior of Watchdog class
 */

#include <Arduino.h>
#include <unity.h>
#include <Watchdog.h>

void test_singleton_same_instance() {
    // Get instance multiple times
    Watchdog& wd1 = Watchdog::getInstance();
    Watchdog& wd2 = Watchdog::getInstance();
    
    // Should be the same instance
    TEST_ASSERT_EQUAL(&wd1, &wd2);
}

void test_singleton_static_methods() {
    // Test static convenience methods
    TEST_ASSERT_TRUE(Watchdog::quickInit(10, false));
    TEST_ASSERT_TRUE(Watchdog::isGloballyInitialized());
    
    // Register task using static method
    TEST_ASSERT_TRUE(Watchdog::quickRegister("TestTask", false, 1000));
    
    // Check global count
    TEST_ASSERT_EQUAL(1, Watchdog::getGlobalTaskCount());
    
    // Feed using static method
    TEST_ASSERT_TRUE(Watchdog::quickFeed());
    
    // Cleanup
    Watchdog::getInstance().deinit();
}

void test_singleton_shared_state() {
    // Initialize through instance
    Watchdog& wd = Watchdog::getInstance();
    wd.init(15, false);
    
    // Register through instance
    wd.registerCurrentTask("Task1", true, 2000);
    
    // Check through static method - should see the registration
    TEST_ASSERT_EQUAL(1, Watchdog::getGlobalTaskCount());
    
    // Register another through static method
    vTaskDelay(pdMS_TO_TICKS(100)); // Brief delay to ensure different task
    Watchdog::quickRegister("Task2", false, 3000);
    
    // Check through instance - should see both
    TEST_ASSERT_EQUAL(2, wd.getRegisteredTaskCount());
    
    // Cleanup
    wd.deinit();
}

void test_multiple_references() {
    // Multiple parts of code can get references
    Watchdog& lib1_wd = Watchdog::getInstance();
    Watchdog& lib2_wd = Watchdog::getInstance();
    Watchdog& user_wd = Watchdog::getInstance();
    
    // All should be the same
    TEST_ASSERT_EQUAL(&lib1_wd, &lib2_wd);
    TEST_ASSERT_EQUAL(&lib2_wd, &user_wd);
    
    // Initialize through one
    lib1_wd.init(20, false);
    
    // Should be initialized for all
    TEST_ASSERT_TRUE(lib2_wd.isInitialized());
    TEST_ASSERT_TRUE(user_wd.isInitialized());
    TEST_ASSERT_TRUE(Watchdog::isGloballyInitialized());
    
    // Cleanup
    user_wd.deinit();
}

void setup() {
    delay(2000);
    UNITY_BEGIN();
    
    RUN_TEST(test_singleton_same_instance);
    RUN_TEST(test_singleton_static_methods);
    RUN_TEST(test_singleton_shared_state);
    RUN_TEST(test_multiple_references);
    
    UNITY_END();
}

void loop() {
    // Empty
}