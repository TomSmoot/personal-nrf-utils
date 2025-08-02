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
│   ├── ble_common/       # BLE service wrappers
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
#include "ble_common/service_manager.h"
```

### Method 2: Git Submodule
Add as submodule to your project:
```bash
git submodule add https://github.com/yourusername/personal-nrf-utils.git utils
```

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