# Watchdog - CLAUDE.md

## Overview
Thread-safe watchdog timer management for ESP32 FreeRTOS tasks. Singleton wrapper around ESP-IDF's task watchdog timer (TWDT) with IWatchdog interface for dependency injection.

## Key Features
- Singleton pattern (ESP-IDF TWDT is global resource)
- IWatchdog interface for DI and testing
- NullWatchdog no-op implementation included
- ESP-IDF v4/v5 API compatibility
- Per-task timeout tracking
- Critical vs non-critical task support

## Architecture (v3.0.0)

### Interface Hierarchy
```
IWatchdog (interface)
    ├── Watchdog (real implementation, singleton)
    └── NullWatchdog (no-op implementation)
```

### IWatchdog Interface
```cpp
class IWatchdog {
    virtual bool init(uint32_t timeoutSeconds, bool panicOnTimeout) = 0;
    virtual bool registerCurrentTask(const char* name, bool critical, uint32_t intervalMs) = 0;
    virtual bool unregisterCurrentTask() = 0;
    virtual bool feed() = 0;
    virtual size_t checkHealth() = 0;
    virtual bool isInitialized() const = 0;
};
```

## Usage

### Direct Usage
```cpp
Watchdog& wd = Watchdog::getInstance();
wd.init(30, true);  // 30s timeout, panic on trigger

void myTask(void* params) {
    wd.registerCurrentTask("MyTask", true, 5000);
    while (true) {
        // Do work
        wd.feed();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

### With TaskManager (recommended)
```cpp
TaskManager tm(&Watchdog::getInstance());
// TaskManager handles registration and feeding
```

### Static Convenience Methods
```cpp
Watchdog::quickInit(30, true);
Watchdog::quickRegister("Task");
Watchdog::quickFeed();
```

## Thread Safety
- Mutex-protected internal state
- Tasks must register/feed from own context
- Atomic flags for initialization state

## Build Configuration
```ini
build_flags =
    -DWATCHDOG_DEBUG  ; Enable debug logging
```
