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
    .nus_data_received = on_data_received,
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
