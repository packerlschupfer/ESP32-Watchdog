/**
 * @file WatchdogLog.h
 * @brief Logging configuration for Watchdog library (C++11 compatible)
 * 
 * This header provides a self-contained logging configuration that works with
 * both ESP-IDF and custom Logger without requiring C++17 features.
 */

#ifndef WATCHDOG_LOG_H
#define WATCHDOG_LOG_H

// Define the log tag for this library
#define WDOG_LOG_TAG "Watchdog"

// Define log levels based on debug flag
#ifdef WATCHDOG_DEBUG
    // Debug mode: Show all levels
    #define WDOG_LOG_LEVEL_E ESP_LOG_ERROR
    #define WDOG_LOG_LEVEL_W ESP_LOG_WARN
    #define WDOG_LOG_LEVEL_I ESP_LOG_INFO
    #define WDOG_LOG_LEVEL_D ESP_LOG_DEBUG
    #define WDOG_LOG_LEVEL_V ESP_LOG_VERBOSE
#else
    // Release mode: Only Error, Warn, Info
    #define WDOG_LOG_LEVEL_E ESP_LOG_ERROR
    #define WDOG_LOG_LEVEL_W ESP_LOG_WARN
    #define WDOG_LOG_LEVEL_I ESP_LOG_INFO
    #define WDOG_LOG_LEVEL_D ESP_LOG_NONE  // Suppress
    #define WDOG_LOG_LEVEL_V ESP_LOG_NONE  // Suppress
#endif

// Route to custom logger or ESP-IDF
#ifdef USE_CUSTOM_LOGGER
    #include <LogInterface.h>
    #define WDOG_LOG_E(...) LOG_WRITE(WDOG_LOG_LEVEL_E, WDOG_LOG_TAG, __VA_ARGS__)
    #define WDOG_LOG_W(...) LOG_WRITE(WDOG_LOG_LEVEL_W, WDOG_LOG_TAG, __VA_ARGS__)
    #define WDOG_LOG_I(...) LOG_WRITE(WDOG_LOG_LEVEL_I, WDOG_LOG_TAG, __VA_ARGS__)
    #define WDOG_LOG_D(...) LOG_WRITE(WDOG_LOG_LEVEL_D, WDOG_LOG_TAG, __VA_ARGS__)
    #define WDOG_LOG_V(...) LOG_WRITE(WDOG_LOG_LEVEL_V, WDOG_LOG_TAG, __VA_ARGS__)
#else
    // ESP-IDF logging with compile-time suppression
    #include <esp_log.h>
    #define WDOG_LOG_E(...) ESP_LOGE(WDOG_LOG_TAG, __VA_ARGS__)
    #define WDOG_LOG_W(...) ESP_LOGW(WDOG_LOG_TAG, __VA_ARGS__)
    #define WDOG_LOG_I(...) ESP_LOGI(WDOG_LOG_TAG, __VA_ARGS__)
    #ifdef WATCHDOG_DEBUG
        #define WDOG_LOG_D(...) ESP_LOGD(WDOG_LOG_TAG, __VA_ARGS__)
        #define WDOG_LOG_V(...) ESP_LOGV(WDOG_LOG_TAG, __VA_ARGS__)
    #else
        #define WDOG_LOG_D(...) ((void)0)
        #define WDOG_LOG_V(...) ((void)0)
    #endif
#endif

#endif // WATCHDOG_LOG_H