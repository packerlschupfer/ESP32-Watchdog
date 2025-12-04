# Watchdog

Singleton-based thread-safe ESP32 task watchdog timer management library with automatic ESP-IDF version compatibility.

## Features

- **Singleton Pattern**: Ensures only one instance manages ESP-IDF's global watchdog
- **Thread-Safe Operations**: All operations protected by mutexes
- **ESP-IDF Compatibility**: Automatic detection and adaptation for v4.x and v5.x
- **Proper Task Registration**: Tasks register from their own execution context
- **Health Monitoring**: Track missed feeds and task health
- **Critical/Non-Critical Tasks**: Support for both panic and non-panic tasks
- **Per-Task Timeouts**: Configurable feed intervals per task
- **C++11 Compatible**: No C++17 features required, works with older toolchains
- **Zero-Overhead Logging**: Optional Logger support without memory penalty

## Why Singleton?

ESP-IDF's task watchdog timer (TWDT) is a **global resource**. Creating multiple Watchdog instances would cause conflicts as each instance would try to manage the same underlying ESP-IDF watchdog. The singleton pattern ensures:
- Only one instance manages the ESP-IDF TWDT
- All libraries and user code share the same watchdog state
- No conflicts between different components using the watchdog

## Why This Library?

Many ESP32 applications struggle with watchdog integration due to:
- Thread context requirements (tasks must register themselves)
- ESP-IDF API changes between versions
- Complex error handling
- Mixed usage patterns
- **Conflicts when multiple libraries try to manage the watchdog**

This library solves these issues with a clean, robust singleton-based API.

## Installation

### PlatformIO
```ini
lib_deps =
    Watchdog
```

## Quick Start

### Option 1: Using Instance Reference

```cpp
#include <Watchdog.h>

// Get the singleton instance
Watchdog& watchdog = Watchdog::getInstance();

void myTask(void* params) {
    // IMPORTANT: Register from within the task
    watchdog.registerCurrentTask("MyTask", true, 2000);
    
    while (true) {
        // Do work...
        
        // Feed watchdog before delays
        watchdog.feed();
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void setup() {
    // Initialize with 30 second timeout
    watchdog.init(30, true);
    
    // Create your tasks
    xTaskCreate(myTask, "MyTask", 4096, NULL, 1, NULL);
}
```

### Option 2: Using Static Methods

```cpp
#include <Watchdog.h>

void myTask(void* params) {
    // Register using static method
    Watchdog::quickRegister("MyTask", true, 2000);
    
    while (true) {
        // Do work...
        
        // Feed using static method
        Watchdog::quickFeed();
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void setup() {
    // Initialize using static method
    Watchdog::quickInit(30, true);
    
    // Create your tasks
    xTaskCreate(myTask, "MyTask", 4096, NULL, 1, NULL);
}
```

## API Reference

### Getting the Instance

```cpp
static Watchdog& getInstance()
```
Get the singleton instance of the Watchdog. Thread-safe initialization is guaranteed.

### Initialization

```cpp
bool init(uint32_t timeoutSeconds = 30, bool panicOnTimeout = true)
static bool quickInit(uint32_t timeoutSeconds = 30, bool panicOnTimeout = true)
```
Initialize the watchdog timer system.

### Task Registration

```cpp
bool registerCurrentTask(const char* taskName, bool isCritical = true, uint32_t feedIntervalMs = 0)
static bool quickRegister(const char* taskName, bool isCritical = true, uint32_t feedIntervalMs = 0)
```
Register the calling task with the watchdog. **Must be called from within the task context.**

### Feeding

```cpp
bool feed()
static bool quickFeed()
```
Reset the watchdog timer for the current task.

### Monitoring

```cpp
size_t checkHealth()
static size_t quickCheckHealth()
```
Check health of all registered tasks. Returns count of unhealthy tasks.

```cpp
bool getTaskInfo(const char* taskName, TaskInfo& info)
```
Get detailed information about a specific task.

```cpp
static bool isGloballyInitialized()
```
Check if the watchdog singleton is initialized.

```cpp
static size_t getGlobalTaskCount()
```
Get the number of registered tasks.

## Important Notes

1. **Thread Context**: Tasks MUST register themselves from their own execution context
2. **Feed Timing**: Always feed the watchdog BEFORE long delays
3. **Initial Feed**: The library automatically feeds after registration
4. **Error Handling**: All functions return bool for error checking

## Example: Complete Application

```cpp
#include <Arduino.h>
#include <Watchdog.h>

// Get singleton instance
Watchdog& watchdog = Watchdog::getInstance();

void criticalTask(void* params) {
    // Critical task - system will panic if it hangs
    watchdog.registerCurrentTask("Critical", true, 1000);
    
    while (true) {
        // Important work
        watchdog.feed();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void normalTask(void* params) {
    // Non-critical task - logged but no panic
    watchdog.registerCurrentTask("Normal", false, 5000);
    
    while (true) {
        // Regular work
        watchdog.feed();
        vTaskDelay(pdMS_TO_TICKS(4000));
    }
}

void monitorTask(void* params) {
    vTaskDelay(pdMS_TO_TICKS(10000)); // Let system stabilize
    
    while (true) {
        size_t unhealthy = watchdog.checkHealth();
        if (unhealthy > 0) {
            Serial.printf("Warning: %d unhealthy tasks!\n", unhealthy);
        }
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}

## Example: Working with TaskManager

Since both Watchdog and TaskManager now use the same singleton instance, they work together seamlessly:

```cpp
#include <Arduino.h>
#include <TaskManager.h>
#include <Watchdog.h>

TaskManager taskManager;

void userTask(void* params) {
    // This task can use Watchdog directly
    Watchdog::quickRegister("UserTask", true, 2000);
    
    while (true) {
        // Do work...
        Watchdog::quickFeed();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void setup() {
    Serial.begin(115200);
    
    // TaskManager initializes watchdog internally
    taskManager.initWatchdog(30, true);
    
    // Both TaskManager tasks and user tasks share the same watchdog
    TaskConfig config = {
        .name = "ManagedTask",
        .function = someFunction,
        .stackSize = 4096,
        .priority = 1,
        .criticalTask = true
    };
    taskManager.createTask(config);
    
    // Create a user task that also uses watchdog
    xTaskCreate(userTask, "UserTask", 4096, NULL, 1, NULL);
    
    // Both tasks are managed by the same watchdog instance!
    Serial.printf("Total tasks registered: %zu\n", Watchdog::getGlobalTaskCount());
}
```

## Complete Example (continued from above)

```cpp
void setup() {
    Serial.begin(115200);
    
    // Initialize watchdog
    if (!watchdog.init(30, true)) {
        Serial.println("Failed to init watchdog!");
        return;
    }
    
    // Create tasks
    xTaskCreate(criticalTask, "Critical", 4096, NULL, 2, NULL);
    xTaskCreate(normalTask, "Normal", 4096, NULL, 1, NULL);
    xTaskCreate(monitorTask, "Monitor", 4096, NULL, 1, NULL);
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}
```

## Thread Safety

This library is designed to be thread-safe:
- Mutex protection for all shared data
- Atomic operations for counters
- Safe task list management

## Logging Configuration

This library supports flexible logging configuration:

### Using ESP-IDF Logging (Default)
No configuration needed. The library will use ESP-IDF logging.

### Using Custom Logger
Define `USE_CUSTOM_LOGGER` in your build flags:
```ini
build_flags = -DUSE_CUSTOM_LOGGER
```

In your main application file:
```cpp
#include <Logger.h>
#include <LogInterfaceImpl.h>  // Include ONCE in main.cpp
#include <Watchdog.h>

void setup() {
    // Initialize Logger
    Logger::getInstance().init(1024);
    Logger::getInstance().setLogLevel(ESP_LOG_DEBUG);
    
    // Now Watchdog will log through custom Logger
    Watchdog watchdog;
    watchdog.init(30, true);
}
```

### Debug Logging
To enable debug/verbose logging for this library:
```ini
build_flags = -DWATCHDOG_DEBUG
```

### Complete Example
```ini
[env:debug]
build_flags = 
    -DUSE_CUSTOM_LOGGER  ; Use custom logger
    -DWATCHDOG_DEBUG     ; Enable debug for this library
```

### Logging Levels
Without `WATCHDOG_DEBUG`, only Error, Warning, and Info logs are shown. With `WATCHDOG_DEBUG`, all log levels including Debug and Verbose are enabled.

### Benefits
- **Production Ready**: No debug overhead in release builds
- **Targeted Debugging**: Enable debug for Watchdog only
- **Zero Memory Overhead**: No Logger singleton unless USE_CUSTOM_LOGGER defined (~17KB saved)
- **Flexible Backend**: Works with ESP-IDF or custom logger
- **C++11 Compatible**: No C++17 features required

## Version History

### 1.1.0
- Migrated to LogInterface for zero-overhead logging
- Replaced WATCHDOG_USE_CUSTOM_LOGGER with standard USE_CUSTOM_LOGGER flag
- Added example showing custom Logger integration
- No Logger dependency required - uses LogInterface pattern

### 1.0.0
- Initial release
- ESP-IDF v4.x and v5.x support
- Thread-safe operations
- Health monitoring