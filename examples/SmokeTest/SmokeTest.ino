/**
 * @file SmokeTest.ino
 * @brief Minimal compile-smoke example for ESP32-Watchdog.
 *
 * Standalone: exercises the concrete Watchdog singleton (the IWatchdog
 * implementation) and the NullWatchdog no-op impl that consumers such as
 * TaskManager depend on. No sibling libraries are pulled in (default logging
 * routes to ESP-IDF esp_log.h, USE_CUSTOM_LOGGER is intentionally undefined).
 */

#include <Arduino.h>
#include "Watchdog.h"   // pulls IWatchdog.h (incl. NullWatchdog)

// Exercise the interface via the no-op implementation TaskManager relies on.
static NullWatchdog g_nullWdt;

static void exerciseInterface(IWatchdog& wdt) {
    (void)wdt.init(30, true);
    (void)wdt.registerCurrentTask("SmokeTask", true, 1000);
    (void)wdt.feed();
    (void)wdt.checkHealth();
    (void)wdt.getRegisteredTaskCount();
    (void)wdt.getTimeoutMs();
    (void)wdt.isInitialized();
}

void setup() {
    Serial.begin(115200);

    // Concrete singleton: ESP-IDF TWDT-backed implementation.
    Watchdog& wdt = Watchdog::getInstance();
    if (wdt.init(30, true)) {
        (void)wdt.registerCurrentTask("LoopTask", true, 1000);
    }

    // Static convenience wrappers.
    (void)Watchdog::quickInit(30, true);
    (void)Watchdog::isGloballyInitialized();

    // No-op IWatchdog implementation (dependency-injection target).
    exerciseInterface(g_nullWdt);
}

void loop() {
    (void)Watchdog::quickFeed();
    (void)g_nullWdt.feed();
    delay(1000);
}
