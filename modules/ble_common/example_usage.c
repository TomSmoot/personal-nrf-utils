/*
 * Copyright (c) 2025 Personal NRF Utils
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file example_usage.c
 * @brief Example usage of the BLE initialization module
 *
 * This file demonstrates how to use the ble_init module to set up
 * Bluetooth Low Energy functionality on Nordic nRF chips.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "ble_init.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* Example application data */
static uint8_t sensor_data[] = "Hello from nRF device!";

/**
 * @brief Called when BLE stack is ready
 */
static void on_ble_ready(void)
{
    LOG_INF("BLE stack is ready and advertising started");
}

/**
 * @brief Called when a device connects
 */
static void on_ble_connected(struct bt_conn *conn)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("Device connected: %s", addr);
    
    /* Send welcome message via NUS */
    ble_nus_send(conn, sensor_data, sizeof(sensor_data) - 1);
}

/**
 * @brief Called when a device disconnects
 */
static void on_ble_disconnected(struct bt_conn *conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("Device disconnected: %s (reason: %u)", addr, reason);
}

/**
 * @brief Called when NUS data is received
 */
static void on_nus_data_received(struct bt_conn *conn, const uint8_t *data, uint16_t len)
{
    LOG_INF("NUS received %u bytes: %.*s", len, len, data);
    
    /* Echo the data back */
    char response[64];
    int resp_len = snprintf(response, sizeof(response), "Echo: %.*s", len, data);
    ble_nus_send(conn, (uint8_t *)response, resp_len);
}

/**
 * @brief Called when NUS data is sent
 */
static void on_nus_data_sent(struct bt_conn *conn)
{
    LOG_DBG("NUS data transmission completed");
}

/**
 * @brief Called when NUS notifications are enabled/disabled
 */
static void on_nus_send_enabled(bool enabled)
{
    LOG_INF("NUS notifications %s", enabled ? "enabled" : "disabled");
}

/**
 * @brief BLE event callbacks structure
 */
static const struct ble_event_callbacks ble_callbacks = {
    .ready = on_ble_ready,
    .connected = on_ble_connected,
    .disconnected = on_ble_disconnected,
    .nus_data_received = on_nus_data_received,
    .nus_data_sent = on_nus_data_sent,
    .nus_send_enabled = on_nus_send_enabled,
};

/**
 * @brief Main application entry point
 */
int main(void)
{
    int err;

    LOG_INF("Starting BLE example application");

    /* Configure BLE initialization */
    struct ble_init_config ble_config = {
        .device_name = "nRF_Example",      /* Device name for advertising */
        .adv_interval_ms = 100,            /* 100ms advertising interval */
        .connectable = true,               /* Enable connections */
        .enable_nus = true,                /* Enable Nordic UART Service */
    };

    /* Initialize BLE stack and services */
    err = ble_init(&ble_config, &ble_callbacks);
    if (err != BLE_INIT_STATUS_SUCCESS) {
        LOG_ERR("BLE initialization failed (err %d)", err);
        return err;
    }

    LOG_INF("BLE initialization successful");

    /* Main application loop */
    while (1) {
        /* Example: Send periodic sensor data if connected */
        if (ble_get_connection_state() == BLE_CONNECTED) {
            static uint32_t counter = 0;
            char data_msg[32];
            int len = snprintf(data_msg, sizeof(data_msg), "Counter: %u", counter++);
            
            /* Send to all connected devices */
            ble_nus_send(NULL, (uint8_t *)data_msg, len);
        }

        /* Print status every 10 seconds */
        enum ble_connection_state state = ble_get_connection_state();
        uint8_t conn_count = ble_get_connection_count();
        
        const char *state_str = (state == BLE_CONNECTED) ? "Connected" :
                               (state == BLE_ADVERTISING) ? "Advertising" : "Disconnected";
        
        LOG_INF("BLE Status: %s, Connections: %u", state_str, conn_count);

        /* Sleep for 10 seconds */
        k_sleep(K_SECONDS(10));
    }

    return 0;
}

/**
 * @brief Alternative minimal usage example
 */
void minimal_ble_setup_example(void)
{
    /* Minimal BLE setup with default configuration */
    struct ble_init_config simple_config = {
        .device_name = "Simple_nRF",
        .adv_interval_ms = 200,
        .connectable = true,
        .enable_nus = true,
    };

    /* Initialize without callbacks for basic functionality */
    int err = ble_init(&simple_config, NULL);
    if (err == BLE_INIT_STATUS_SUCCESS) {
        LOG_INF("Simple BLE setup complete");
    }
}

/**
 * @brief Example of controlling advertising manually
 */
void advertising_control_example(void)
{
    /* Stop advertising */
    int err = ble_advertising_stop();
    if (err == 0) {
        LOG_INF("Advertising stopped");
        
        /* Wait a bit, then restart */
        k_sleep(K_SECONDS(5));
        
        err = ble_advertising_start();
        if (err == 0) {
            LOG_INF("Advertising restarted");
        }
    }
}

/**
 * @brief Example of disconnecting all devices
 */
void disconnect_all_example(void)
{
    uint8_t count = ble_get_connection_count();
    if (count > 0) {
        LOG_INF("Disconnecting %u device(s)", count);
        int err = ble_disconnect_all();
        if (err == 0) {
            LOG_INF("All devices disconnected");
        }
    }
}
