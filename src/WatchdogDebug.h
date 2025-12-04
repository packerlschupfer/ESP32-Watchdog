/**
 * @file WatchdogDebug.h
 * @brief Advanced debug utilities for Watchdog library
 * 
 * Provides additional debug macros for performance timing, buffer dumps,
 * and detailed diagnostics - all with zero overhead in release builds.
 */

#ifndef WATCHDOG_DEBUG_H
#define WATCHDOG_DEBUG_H

#include "WatchdogLog.h"

// Performance timing macros
#ifdef WATCHDOG_DEBUG
    #define WDOG_TIME_START() unsigned long _wdog_start = millis()
    #define WDOG_TIME_END(msg) WDOG_LOG_D("Timing: %s took %lu ms", msg, millis() - _wdog_start)
#else
    #define WDOG_TIME_START() ((void)0)
    #define WDOG_TIME_END(msg) ((void)0)
#endif

// Task info dump
#ifdef WATCHDOG_DEBUG
    #define WDOG_DUMP_TASK_INFO(info) do { \
        WDOG_LOG_D("Task Info: %s", info.taskName); \
        WDOG_LOG_D("  Handle: %p", info.taskHandle); \
        WDOG_LOG_D("  Critical: %s", info.isCritical ? "Yes" : "No"); \
        WDOG_LOG_D("  Feed Interval: %lu ms", info.feedIntervalMs); \
        WDOG_LOG_D("  Last Feed: %lu ticks ago", xTaskGetTickCount() - info.lastFeedTime); \
        WDOG_LOG_D("  Missed Feeds: %lu", info.missedFeeds); \
    } while(0)
#else
    #define WDOG_DUMP_TASK_INFO(info) ((void)0)
#endif

// Detailed state logging
#ifdef WATCHDOG_DEBUG
    #define WDOG_LOG_STATE(msg) do { \
        WDOG_LOG_D("%s - State: init=%d, tasks=%zu", \
            msg, isInitialized.load(), subscribedTasks.size()); \
    } while(0)
#else
    #define WDOG_LOG_STATE(msg) ((void)0)
#endif

// Feature-specific debug flags (can be enabled individually)
#ifdef WATCHDOG_DEBUG
    // Enable all debug features
    #define WATCHDOG_DEBUG_REGISTRATION  // Task registration details
    #define WATCHDOG_DEBUG_FEEDING       // Feed operation details
    #define WATCHDOG_DEBUG_HEALTH        // Health check details
#endif

// Registration debugging
#ifdef WATCHDOG_DEBUG_REGISTRATION
    #define WDOG_LOG_REG(...) WDOG_LOG_D("REG: " __VA_ARGS__)
#else
    #define WDOG_LOG_REG(...) ((void)0)
#endif

// Feed debugging
#ifdef WATCHDOG_DEBUG_FEEDING
    #define WDOG_LOG_FEED(...) WDOG_LOG_V("FEED: " __VA_ARGS__)
#else
    #define WDOG_LOG_FEED(...) ((void)0)
#endif

// Health check debugging
#ifdef WATCHDOG_DEBUG_HEALTH
    #define WDOG_LOG_HEALTH(...) WDOG_LOG_D("HEALTH: " __VA_ARGS__)
#else
    #define WDOG_LOG_HEALTH(...) ((void)0)
#endif

#endif // WATCHDOG_DEBUG_H