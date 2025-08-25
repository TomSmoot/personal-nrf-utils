/*
 * Copyright (c) 2025 Personal NRF Utils
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BLE_INIT_H_
#define BLE_INIT_H_

/**
 * @file
 * @brief BLE initialization module for Nordic nRF5340 application core
 *
 * This module provides BLE initialization for nRF5340 application core
 * using IPC communication with the network core running BLE stack.
 * 
 * Network core should be running the nRF BLE SPI example or similar
 * BLE-enabled firmware that provides BLE services via IPC.
 */

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief BLE initialization status codes */
enum ble_init_status {
    BLE_INIT_STATUS_SUCCESS = 0,
    BLE_INIT_STATUS_IPC_FAILED = -1,
    BLE_INIT_STATUS_TIMEOUT = -2,
    BLE_INIT_STATUS_ALREADY_INITIALIZED = -3,
    BLE_INIT_STATUS_NOT_SUPPORTED = -4,
};

/** @brief BLE connection state */
enum ble_connection_state {
    BLE_DISCONNECTED = 0,
    BLE_CONNECTED,
    BLE_ADVERTISING,
    BLE_IPC_ERROR,
};

/** @brief BLE initialization configuration for nRF5340 app core */
struct ble_init_config {
    /** Device name for advertising (max 29 characters) */
    const char *device_name;
    
    /** Advertising interval in milliseconds (20-10240 ms) */
    uint16_t adv_interval_ms;
    
    /** Whether to enable connectable advertising */
    bool connectable;
    
    /** Whether to enable UART-like service */
    bool enable_uart_service;
};

/** @brief BLE event callbacks */
struct ble_event_callbacks {
    /** @brief Called when BLE IPC communication is ready */
    void (*ready)(void);
    
    /** @brief Called when a device connects (simulated) */
    void (*connected)(void);
    
    /** @brief Called when a device disconnects (simulated) */
    void (*disconnected)(uint8_t reason);
    
    /** @brief Called when data is received via IPC from network core */
    void (*data_received)(const uint8_t *data, uint16_t len);
    
    /** @brief Called when data transmission is complete */
    void (*data_sent)(void);
};

/**
 * @brief Initialize BLE communication via IPC with network core
 *
 * This function sets up IPC communication with the network core
 * which should be running BLE stack firmware.
 *
 * @param config Configuration for BLE initialization
 * @param callbacks Event callbacks (can be NULL)
 *
 * @return BLE_INIT_STATUS_SUCCESS on success, negative error code otherwise
 */
int ble_init(const struct ble_init_config *config, const struct ble_event_callbacks *callbacks);

/**
 * @brief Send data to network core for BLE transmission
 *
 * @param data Data buffer to send
 * @param len Length of data
 *
 * @return 0 on success, negative error code otherwise
 */
int ble_send_data(const uint8_t *data, uint16_t len);

/**
 * @brief Get current connection state (simulated based on IPC)
 *
 * @return Current BLE connection state
 */
enum ble_connection_state ble_get_connection_state(void);

/**
 * @brief Check if BLE IPC communication is working
 *
 * @return true if IPC is functional, false otherwise
 */
bool ble_is_ipc_ready(void);

/**
 * @brief Send a simple test message to verify IPC communication
 *
 * @return 0 on success, negative error code otherwise
 */
int ble_test_ipc_communication(void);

#ifdef __cplusplus
}
#endif

#endif /* BLE_INIT_H_ */