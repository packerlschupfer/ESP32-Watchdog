/**
 * @file IWatchdog.h
 * @brief Abstract interface for watchdog timer management
 *
 * This interface enables dependency injection for testability and
 * allows alternative watchdog implementations (mock, null, proxy).
 *
 * @author TaskManager Contributors
 * @date 2025-11-30
 */

#ifndef IWATCHDOG_H
#define IWATCHDOG_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cstdint>
#include <cstddef>

/**
 * @interface IWatchdog
 * @brief Abstract interface for watchdog timer operations
 *
 * Consumers should depend on this interface rather than the concrete
 * Watchdog implementation to enable testing and flexibility.
 */
class IWatchdog {
public:
    /**
     * @brief Virtual destructor for proper cleanup
     */
    virtual ~IWatchdog() = default;

    // ============== Lifecycle Methods ==============

    /**
     * @brief Initialize the watchdog timer
     * @param timeoutSeconds Timeout in seconds before watchdog triggers
     * @param panicOnTimeout If true, system will panic/reset on timeout
     * @return true if initialization successful
     */
    [[nodiscard]] virtual bool init(uint32_t timeoutSeconds = 30,
                                    bool panicOnTimeout = true) noexcept = 0;

    /**
     * @brief Deinitialize the watchdog timer
     * @return true if deinitialization successful
     */
    virtual bool deinit() noexcept = 0;

    // ============== Task Registration ==============

    /**
     * @brief Register current task with watchdog
     * @param taskName Name for identification (max configMAX_TASK_NAME_LEN)
     * @param isCritical If true, timeout will trigger panic
     * @param feedIntervalMs Expected feed interval (0 = auto-calculate)
     * @return true if registration successful
     * @note MUST be called from the task's own execution context
     */
    [[nodiscard]] virtual bool registerCurrentTask(
        const char* taskName,
        bool isCritical = true,
        uint32_t feedIntervalMs = 0) noexcept = 0;

    /**
     * @brief Unregister current task from watchdog
     * @return true if unregistration successful
     */
    virtual bool unregisterCurrentTask() noexcept = 0;

    /**
     * @brief Unregister a task by its handle
     * @param taskHandle Handle of the task to unregister
     * @param taskName Optional name for logging (nullptr = lookup name)
     * @return true if unregistration successful
     * @note Can be called from any task context
     */
    virtual bool unregisterTaskByHandle(
        TaskHandle_t taskHandle,
        const char* taskName = nullptr) noexcept = 0;

    // ============== Runtime Operations ==============

    /**
     * @brief Feed the watchdog for current task
     * @return true if feed successful
     * @note MUST be called from registered task context
     */
    [[nodiscard]] virtual bool feed() noexcept = 0;

    /**
     * @brief Check health of all registered tasks
     * @return Number of tasks that haven't fed watchdog recently
     */
    [[nodiscard]] virtual size_t checkHealth() noexcept = 0;

    // ============== Status Queries ==============

    /**
     * @brief Check if watchdog is initialized
     * @return true if initialized
     */
    [[nodiscard]] virtual bool isInitialized() const noexcept = 0;

    /**
     * @brief Get current timeout in milliseconds
     * @return Timeout in milliseconds
     */
    [[nodiscard]] virtual uint32_t getTimeoutMs() const noexcept = 0;

    /**
     * @brief Get number of registered tasks
     * @return Number of tasks registered with watchdog
     */
    [[nodiscard]] virtual size_t getRegisteredTaskCount() const noexcept = 0;
};

/**
 * @class NullWatchdog
 * @brief No-op implementation for disabled watchdog scenarios
 *
 * Use this when watchdog functionality should be disabled
 * but code paths must still execute without nullptr checks.
 * All operations succeed silently with no side effects.
 */
class NullWatchdog final : public IWatchdog {
public:
    bool init(uint32_t, bool) noexcept override { return true; }
    bool deinit() noexcept override { return true; }
    bool registerCurrentTask(const char*, bool, uint32_t) noexcept override { return true; }
    bool unregisterCurrentTask() noexcept override { return true; }
    bool unregisterTaskByHandle(TaskHandle_t, const char*) noexcept override { return true; }
    bool feed() noexcept override { return true; }
    size_t checkHealth() noexcept override { return 0; }
    bool isInitialized() const noexcept override { return true; }
    uint32_t getTimeoutMs() const noexcept override { return 0; }
    size_t getRegisteredTaskCount() const noexcept override { return 0; }
};

#endif // IWATCHDOG_H
