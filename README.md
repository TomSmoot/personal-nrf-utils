# Personal nRF Utils

A collection of reusable modules and utilities for Nordic nRF development using Zephyr RTOS. This repo provides clean abstractions and common patterns for embedded development with nRF52/nRF53 chips.

## Purpose

This repository serves as:
- Learning resource for Nordic nRF SDK and Zephyr development
- Reusable code modules for multiple projects
- Reference implementations of common embedded patterns
- Cross-platform development support (desktop + tablet)

## Repository Structure

```
personal-nrf-utils/
├── sdk-headers/           # Nordic/Zephyr headers for tablet IntelliSense
│   ├── zephyr/           # Core Zephyr RTOS headers
│   │   ├── drivers/      # Hardware driver interfaces
│   │   └── kernel.h      # Core Zephyr functions
│   └── bluetooth/        # Nordic BLE service headers
├── modules/              # Reusable utility modules
│   ├── gpio_helpers/     # GPIO control abstractions
│   ├── ble_common/       # BLE initialization and service management
│   │   ├── ble_init.h    # BLE stack initialization API
│   │   ├── ble_init.c    # BLE initialization implementation
│   │   └── example_usage.c # Usage examples and patterns
│   ├── nrf_utils/        # Nordic chip utility functions
│   │   ├── nrf_utils.h   # Battery, temperature, system info API
│   │   └── nrf_utils.c   # Utility functions implementation
│   ├── cmd_parser/       # Command line parser for BLE serial
│   │   ├── cmd_parser.h  # Command processing API
│   │   └── cmd_parser.c  # Command parser implementation
│   └── uart_helpers/     # UART communication utilities
├── docs/                 # Documentation and notes
└── README.md
```

## Development Setup

### Desktop Development (Full Build Environment)
- Requires Nordic Connect SDK installed via VS Code nRF Connect extension
- Full ARM GCC toolchain for compilation and flashing
- Used for testing, building, and hardware validation

### Tablet Development (Code Editing Only)
- Uses `sdk-headers/` folder for IntelliSense support
- GitHub web editor or VS Code web for editing
- No compilation capability - sync with desktop for builds

## SDK Headers

The `sdk-headers/` directory contains select header files from:
- **Zephyr RTOS**: Core system and driver headers
- **Nordic SDK**: BLE services and nRF-specific functionality

These headers provide IntelliSense support for tablet development without requiring the full Nordic SDK installation.

### Header Sources
- Zephyr headers: https://github.com/zephyrproject-rtos/zephyr
- Nordic headers: https://github.com/nrfconnect/sdk-nrf

## Usage in Projects

### Method 1: Copy Modules
Copy specific modules from this repo into your project:
```c
#include "gpio_helpers/led_controller.h"
#include "ble_common/ble_init.h"
```

### Method 2: Git Submodule
Add as submodule to your project:
```bash
git submodule add https://github.com/yourusername/personal-nrf-utils.git utils
```

### BLE Module Usage Example
```c
#include "ble_common/ble_init.h"
#include "nrf_utils/nrf_utils.h"
#include "cmd_parser/cmd_parser.h"

/* Initialize all modules */
nrf_utils_init();
cmd_parser_init();

/* Configure BLE initialization */
struct ble_init_config ble_config = {
    .device_name = "My_nRF_Device",
    .adv_interval_ms = 100,
    .connectable = true,
    .enable_nus = true,  /* Nordic UART Service */
};

/* Set up event callbacks */
static const struct ble_event_callbacks ble_callbacks = {
    .ready = on_ble_ready,
    .connected = on_connected,
    .disconnected = on_disconnected,
    .nus_data_received = cmd_parser_process, /* Process commands */
};

/* Initialize BLE stack */
int err = ble_init(&ble_config, &ble_callbacks);
```

## Module Documentation

### BLE Common Module (`modules/ble_common/`)

The BLE initialization module provides a high-level API for setting up Bluetooth Low Energy functionality on Nordic nRF chips. It handles:

- **Bluetooth Stack Initialization**: Automated setup of the Zephyr Bluetooth stack
- **Advertising Management**: Configurable advertising with device name and intervals  
- **Nordic UART Service (NUS)**: Optional UART-over-BLE service for data exchange
- **Connection Management**: Automatic tracking of multiple BLE connections
- **Event Callbacks**: Clean callback system for BLE events

#### Features
- Simple one-function initialization
- Automatic advertising restart after disconnection
- Support for multiple simultaneous connections
- Built-in logging and error handling
- Configurable device name and advertising parameters
- Optional NUS service with data echo capabilities

#### Dependencies
- Zephyr RTOS Bluetooth subsystem
- Nordic UART Service (for NUS functionality)
- Zephyr settings subsystem (optional, for persistent storage)

#### Example Usage
See `modules/ble_common/example_usage.c` for complete examples including:
- Basic BLE setup with callbacks
- Minimal configuration setup
- Manual advertising control
- Data transmission via NUS
- Connection management

### nRF Utils Module (`modules/nrf_utils/`)

The nRF utilities module provides common system functions for Nordic nRF chips:

- **Battery Monitoring**: Read battery voltage and calculate percentage
- **Temperature Reading**: Access internal temperature sensor
- **System Information**: Get board name, SoC info, uptime, memory stats
- **Power Management**: System reset and deep sleep functions

#### Features
- ADC-based battery voltage monitoring with percentage calculation
- Internal temperature sensor access
- System uptime and memory usage tracking
- Clean API for common system information
- Power management utilities

#### Dependencies
- Zephyr ADC subsystem (for battery monitoring)
- Zephyr sensor subsystem (for temperature)
- Zephyr system APIs

### Command Parser Module (`modules/cmd_parser/`)

The command parser module provides a command-line interface over BLE NUS:

- **Serial Command Processing**: Parse commands received via BLE
- **Built-in Commands**: System status, battery info, LED control, etc.
- **Extensible Design**: Easy to add new commands
- **Response Handling**: Automatic response transmission over BLE

#### Available Commands
- `help` - Show all available commands
- `status` - Complete system status (uptime, battery, temperature, connections)
- `battery` - Detailed battery information
- `temp` - Current temperature reading
- `info` - System information (board, SoC, memory)
- `uptime` - Formatted uptime display
- `led on/off/toggle` - LED control
- `echo <text>` - Echo test
- `reset` - System reset

#### Features
- Line-buffered command processing
- Automatic response generation
- Memory-efficient command parsing
- Extensible command table
- Integration with nRF utils for system info

### Complete Test Application

The included `main.c` demonstrates a complete test application featuring:

- **Full System Integration**: All modules working together
- **BLE Serial Interface**: Connect with Android/iOS BLE terminal apps
- **Interactive Commands**: Test all system functions via BLE commands
- **Automatic Status Updates**: Periodic status reports every 10 seconds
- **LED Indicators**: Visual connection status feedback
- **Production-Ready Structure**: Clean separation of concerns

## Module Development Guidelines

- Keep modules focused and single-purpose
- Provide clean APIs that abstract Nordic/Zephyr complexity
- Include example usage in module headers
- Document hardware requirements and dependencies
- Test modules with actual hardware before committing

## Workflow

1. **Learn**: Study Nordic SDK examples and Zephyr documentation
2. **Abstract**: Create reusable modules from common patterns
3. **Test**: Validate modules with hardware on desktop
4. **Document**: Update this README and module documentation
5. **Reuse**: Copy proven modules into actual projects

## License

Private repository for personal/research use. Consider appropriate licensing before making public.

## Notes

- Headers in `sdk-headers/` are copies for development convenience only
- Always use official Nordic SDK for actual compilation
- Keep IP boundaries clear when using in multiple projects
- This repo assumes Nordic Connect SDK v2.x compatibility
