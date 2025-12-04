# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - 2025-12-04

### Added
- Initial public release
- Watchdog singleton class wrapping ESP-IDF Task Watchdog Timer (TWDT)
- IWatchdog interface for dependency injection and testing
- NullWatchdog no-op implementation included
- ESP-IDF v4.x and v5.x compatibility with automatic API detection
- Per-task watchdog registration from correct thread context
- Task health monitoring and diagnostics
- Support for critical and non-critical tasks
- Configurable feed intervals per task
- Static convenience methods (quickInit, quickFeed, quickRegister)
- Thread-safe operations with mutex protection
- Atomic counters for missed feed tracking
- Optional debug logging with WATCHDOG_DEBUG macro

### Notes
- Production-tested in FreeRTOS environment with 16 concurrent tasks
- Zero watchdog resets over weeks of continuous operation
- Previous internal versions (v1.x-v3.x) not publicly released
- Reset to v0.1.0 for clean public release start
