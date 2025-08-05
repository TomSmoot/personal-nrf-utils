/*
 * Copyright (c) 2025 Personal NRF Utils
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BLE_INIT_H_
#define BLE_INIT_H_

/**
 * @file
 * @brief BLE initialization module for Nordic nRF chips
 *
 * This module provides comprehensive BLE initialization including:
 * - Bluetooth stack initialization
 * - Advertising setup
 * - Nordic UART Service (NUS) initialization
 * - Connection management
 */

#include <zephyr/types.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief BLE initialization status codes */
enum ble_init_status {
    BLE_INIT_STATUS_SUCCESS = 0,
    BLE_INIT_STATUS_STACK_FAILED = -1,
    BLE_INIT_STATUS_ADVERTISING_FAILED = -2,
    BLE_INIT_STATUS_NUS_FAILED = -3,
    BLE_INIT_STATUS_ALREADY_INITIALIZED = -4,
};

/** @brief BLE connection state */
enum ble_connection_state {
    BLE_DISCONNECTED = 0,
    BLE_CONNECTED,
    BLE_ADVERTISING,
};

/** @brief BLE initialization configuration */
struct ble_init_config {
    /** Device name for advertising (max 29 characters) */
    const char *device_name;
    
    /** Advertising interval in milliseconds (20-10240 ms) */
    uint16_t adv_interval_ms;
    
    /** Whether to enable connectable advertising */
    bool connectable;
    
    /** Whether to initialize NUS service */
    bool enable_nus;
};

/** @brief BLE event callbacks */
struct ble_event_callbacks {
    /** @brief Called when BLE stack is ready */
    void (*ready)(void);
    
    /** @brief Called when a device connects */
    void (*connected)(struct bt_conn *conn);
    
    /** @brief Called when a device disconnects */
    void (*disconnected)(struct bt_conn *conn, uint8_t reason);
    
    /** @brief Called when NUS data is received */
    void (*nus_data_received)(struct bt_conn *conn, const uint8_t *data, uint16_t len);
    
    /** @brief Called when NUS data is sent */
    void (*nus_data_sent)(struct bt_conn *conn);
    
    /** @brief Called when NUS notifications are enabled/disabled */
    void (*nus_send_enabled)(bool enabled);
};

/**
 * @brief Initialize BLE stack and services
 *
 * This function initializes the Bluetooth stack, sets up advertising,
 * and optionally initializes the Nordic UART Service.
 *
 * @param config Configuration for BLE initialization
 * @param callbacks Event callbacks (can be NULL)
 *
 * @return BLE_INIT_STATUS_SUCCESS on success, negative error code otherwise
 */
int ble_init(const struct ble_init_config *config, const struct ble_event_callbacks *callbacks);

/**
 * @brief Start advertising
 *
 * @return 0 on success, negative error code otherwise
 */
int ble_advertising_start(void);

/**
 * @brief Stop advertising
 *
 * @return 0 on success, negative error code otherwise
 */
int ble_advertising_stop(void);

/**
 * @brief Send data via NUS
 *
 * @param conn Connection to send data to (NULL for all connections)
 * @param data Data buffer to send
 * @param len Length of data
 *
 * @return 0 on success, negative error code otherwise
 */
int ble_nus_send(struct bt_conn *conn, const uint8_t *data, uint16_t len);

/**
 * @brief Get current connection state
 *
 * @return Current BLE connection state
 */
enum ble_connection_state ble_get_connection_state(void);

/**
 * @brief Get number of active connections
 *
 * @return Number of active BLE connections
 */
uint8_t ble_get_connection_count(void);

/**
 * @brief Disconnect from a specific connection
 *
 * @param conn Connection to disconnect
 *
 * @return 0 on success, negative error code otherwise
 */
int ble_disconnect(struct bt_conn *conn);

/**
 * @brief Disconnect from all connections
 *
 * @return 0 on success, negative error code otherwise
 */
int ble_disconnect_all(void);

#ifdef __cplusplus
}
#endif

#endif /* BLE_INIT_H_ */