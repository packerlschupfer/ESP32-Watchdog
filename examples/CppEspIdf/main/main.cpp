/**
 * @file main.cpp
 * @brief Native ESP-IDF example exercising the Watchdog library.
 *
 * Builds under pure ESP-IDF (no Arduino API) on both IDF v4.4.x and v5.x.
 * The Watchdog class wraps the ESP-IDF Task Watchdog Timer (TWDT) and
 * adapts to the esp_task_wdt API differences between v4 and v5 internally.
 */

#include <cstdio>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "Watchdog.h"
#include "IWatchdog.h"

static const char *TAG = "MAIN";

// A worker task that registers itself with the watchdog and feeds it.
static void worker_task(void *pvParameters)
{
    Watchdog &watchdog = Watchdog::getInstance();

    // Register the current task (MUST be done from the task's own context).
    if (watchdog.registerCurrentTask("worker_task", /*isCritical=*/true,
                                     /*feedIntervalMs=*/1000)) {
        ESP_LOGI(TAG, "worker_task registered with watchdog");
    } else {
        ESP_LOGE(TAG, "worker_task failed to register with watchdog");
    }

    ESP_LOGI(TAG, "Registered task count: %u",
             static_cast<unsigned>(watchdog.getRegisteredTaskCount()));

    int iterations = 0;
    while (true) {
        // Do work, then feed the watchdog.
        watchdog.feed();

        // Periodically check overall health.
        if ((iterations % 5) == 0) {
            size_t unhealthy = watchdog.checkHealth();
            ESP_LOGI(TAG, "Health check: %u unhealthy task(s)",
                     static_cast<unsigned>(unhealthy));
        }

        ++iterations;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Watchdog CppEspIdf example starting");

    // Demonstrate the NullWatchdog (no-op) implementation via the interface.
    NullWatchdog nullWdog;
    IWatchdog &iface = nullWdog;
    (void)iface.init(10, false);
    (void)iface.feed();

    // Initialize the real watchdog: 5 second timeout, no panic (CI-friendly).
    Watchdog &watchdog = Watchdog::getInstance();
    if (watchdog.init(/*timeoutSeconds=*/5, /*panicOnTimeout=*/false)) {
        ESP_LOGI(TAG, "Watchdog initialized, timeout=%lu ms",
                 static_cast<unsigned long>(watchdog.getTimeoutMs()));
    } else {
        ESP_LOGE(TAG, "Watchdog init failed");
    }

    xTaskCreate(&worker_task, "worker_task", 4096, nullptr, 5, nullptr);
}
