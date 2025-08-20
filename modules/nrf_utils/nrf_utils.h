/*
 * Copyright (c) 2025 Personal NRF Utils
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRF_UTILS_H_
#define NRF_UTILS_H_

/**
 * @file
 * @brief Nordic nRF utility functions
 *
 * This module provides common utility functions for Nordic nRF chips:
 * - Battery voltage monitoring
 * - System information
 * - Temperature reading
 * - Reset and power management
 */

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief System information structure */
struct nrf_system_info {
    const char *board_name;
    const char *soc_name;
    uint32_t uptime_ms;
    uint32_t free_heap_bytes;
    uint8_t reset_reason;
};

/** @brief Battery status structure */
struct nrf_battery_status {
    uint16_t voltage_mv;        /* Battery voltage in millivolts */
    uint8_t percentage;         /* Battery percentage (0-100) */
    bool is_charging;           /* True if battery is charging */
    bool is_present;            /* True if battery is present */
};

/**
 * @brief Initialize nRF utilities
 *
 * @return 0 on success, negative error code otherwise
 */
int nrf_utils_init(void);

/**
 * @brief Get battery status
 *
 * @param status Pointer to battery status structure
 * @return 0 on success, negative error code otherwise
 */
int nrf_get_battery_status(struct nrf_battery_status *status);

/**
 * @brief Get battery voltage in millivolts
 *
 * @return Battery voltage in mV, or negative error code
 */
int nrf_get_battery_voltage_mv(void);

/**
 * @brief Get battery percentage (0-100)
 *
 * @return Battery percentage, or negative error code
 */
int nrf_get_battery_percentage(void);

/**
 * @brief Get system temperature in Celsius
 *
 * @return Temperature in degrees Celsius, or negative error code
 */
int nrf_get_temperature_celsius(void);

/**
 * @brief Get system information
 *
 * @param info Pointer to system info structure
 * @return 0 on success, negative error code otherwise
 */
int nrf_get_system_info(struct nrf_system_info *info);

/**
 * @brief Get system uptime in milliseconds
 *
 * @return Uptime in milliseconds
 */
uint32_t nrf_get_uptime_ms(void);

/**
 * @brief Get free heap memory in bytes
 *
 * @return Free heap bytes, or 0 if heap tracking disabled
 */
uint32_t nrf_get_free_heap_bytes(void);

/**
 * @brief Reset the system
 */
void nrf_system_reset(void);

/**
 * @brief Enter deep sleep mode
 *
 * @param duration_ms Sleep duration in milliseconds (0 = indefinite)
 */
void nrf_deep_sleep(uint32_t duration_ms);

#ifdef __cplusplus
}
#endif

#endif /* NRF_UTILS_H_ */
