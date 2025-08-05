/*
 * Copyright (c) 2025 Personal NRF Utils
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ble_init.h"
#include <bluetooth/services/nus.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

LOG_MODULE_REGISTER(ble_init, LOG_LEVEL_DBG);

/* Default device name if none provided */
#define DEFAULT_DEVICE_NAME "Nordic_Device"

/* Default advertising interval (100ms) */
#define DEFAULT_ADV_INTERVAL_MS 100

/* Maximum device name length for advertising */
#define MAX_DEVICE_NAME_LEN 29

/* Static variables for module state */
static bool ble_initialized = false;
static enum ble_connection_state current_state = BLE_DISCONNECTED;
static struct bt_conn *current_connections[CONFIG_BT_MAX_CONN];
static uint8_t connection_count = 0;
static struct ble_init_config stored_config;
static const struct ble_event_callbacks *event_callbacks = NULL;

/* Advertising data */
static struct bt_data ad_data[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, NULL, 0), /* Will be set dynamically */
};

static struct bt_data sd_data[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL),
};

/* Forward declarations */
static void bt_ready_callback(int err);
static void connected_callback(struct bt_conn *conn, uint8_t err);
static void disconnected_callback(struct bt_conn *conn, uint8_t reason);
static void nus_received_callback(struct bt_conn *conn, const uint8_t *const data, uint16_t len);
static void nus_sent_callback(struct bt_conn *conn);
static void nus_send_enabled_callback(enum bt_nus_send_status status);

/* Connection callbacks */
BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected_callback,
    .disconnected = disconnected_callback,
};

/* NUS callbacks */
static struct bt_nus_cb nus_callbacks = {
    .received = nus_received_callback,
    .sent = nus_sent_callback,
    .send_enabled = nus_send_enabled_callback,
};

/* Advertising parameters */
static struct bt_le_adv_param adv_param = {
    .id = BT_ID_DEFAULT,
    .sid = 0,
    .secondary_max_skip = 0,
    .options = BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME,
    .interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
    .interval_max = BT_GAP_ADV_FAST_INT_MAX_2,
    .peer = NULL,
};

/**
 * @brief Bluetooth ready callback
 */
static void bt_ready_callback(int err)
{
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return;
    }

    LOG_INF("Bluetooth initialized");

    /* Load settings if enabled */
#ifdef CONFIG_BT_SETTINGS
    settings_load();
#endif

    /* Initialize NUS if enabled */
    if (stored_config.enable_nus) {
        err = bt_nus_init(&nus_callbacks);
        if (err) {
            LOG_ERR("NUS initialization failed (err %d)", err);
            return;
        }
        LOG_INF("NUS service initialized");
    }

    /* Start advertising */
    err = ble_advertising_start();
    if (err) {
        LOG_ERR("Advertising failed to start (err %d)", err);
        return;
    }

    /* Call ready callback if provided */
    if (event_callbacks && event_callbacks->ready) {
        event_callbacks->ready();
    }
}

/**
 * @brief Connection established callback
 */
static void connected_callback(struct bt_conn *conn, uint8_t err)
{
    char addr[BT_ADDR_LE_STR_LEN];

    if (err) {
        LOG_ERR("Connection failed (err %d)", err);
        return;
    }

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("Connected: %s", addr);

    /* Store connection reference */
    for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
        if (current_connections[i] == NULL) {
            current_connections[i] = bt_conn_ref(conn);
            connection_count++;
            break;
        }
    }

    current_state = BLE_CONNECTED;

    /* Call connected callback if provided */
    if (event_callbacks && event_callbacks->connected) {
        event_callbacks->connected(conn);
    }
}

/**
 * @brief Connection terminated callback
 */
static void disconnected_callback(struct bt_conn *conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("Disconnected: %s (reason %u)", addr, reason);

    /* Remove connection reference */
    for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
        if (current_connections[i] == conn) {
            bt_conn_unref(current_connections[i]);
            current_connections[i] = NULL;
            connection_count--;
            break;
        }
    }

    /* Update state */
    if (connection_count == 0) {
        current_state = BLE_DISCONNECTED;
        
        /* Restart advertising */
        int err = ble_advertising_start();
        if (err) {
            LOG_ERR("Failed to restart advertising (err %d)", err);
        }
    }

    /* Call disconnected callback if provided */
    if (event_callbacks && event_callbacks->disconnected) {
        event_callbacks->disconnected(conn, reason);
    }
}

/**
 * @brief NUS data received callback
 */
static void nus_received_callback(struct bt_conn *conn, const uint8_t *const data, uint16_t len)
{
    LOG_DBG("NUS received %u bytes", len);

    /* Call NUS data received callback if provided */
    if (event_callbacks && event_callbacks->nus_data_received) {
        event_callbacks->nus_data_received(conn, data, len);
    }
}

/**
 * @brief NUS data sent callback
 */
static void nus_sent_callback(struct bt_conn *conn)
{
    LOG_DBG("NUS data sent");

    /* Call NUS data sent callback if provided */
    if (event_callbacks && event_callbacks->nus_data_sent) {
        event_callbacks->nus_data_sent(conn);
    }
}

/**
 * @brief NUS send enabled/disabled callback
 */
static void nus_send_enabled_callback(enum bt_nus_send_status status)
{
    bool enabled = (status == BT_NUS_SEND_STATUS_ENABLED);
    LOG_INF("NUS notifications %s", enabled ? "enabled" : "disabled");

    /* Call NUS send enabled callback if provided */
    if (event_callbacks && event_callbacks->nus_send_enabled) {
        event_callbacks->nus_send_enabled(enabled);
    }
}

/**
 * @brief Update advertising interval
 */
static void update_advertising_interval(uint16_t interval_ms)
{
    /* Convert milliseconds to advertising units (0.625ms units) */
    uint16_t interval_units = (interval_ms * 8) / 5;
    
    /* Ensure within valid range */
    if (interval_units < BT_GAP_ADV_FAST_INT_MIN_1) {
        interval_units = BT_GAP_ADV_FAST_INT_MIN_1;
    } else if (interval_units > BT_GAP_ADV_SLOW_INT_MAX) {
        interval_units = BT_GAP_ADV_SLOW_INT_MAX;
    }

    adv_param.interval_min = interval_units;
    adv_param.interval_max = interval_units;
}

/* Public API Implementation */

int ble_init(const struct ble_init_config *config, const struct ble_event_callbacks *callbacks)
{
    int err;

    if (ble_initialized) {
        LOG_WRN("BLE already initialized");
        return BLE_INIT_STATUS_ALREADY_INITIALIZED;
    }

    if (!config) {
        LOG_ERR("Invalid configuration");
        return BLE_INIT_STATUS_STACK_FAILED;
    }

    /* Store configuration and callbacks */
    stored_config = *config;
    event_callbacks = callbacks;

    /* Set device name for advertising */
    const char *device_name = config->device_name ? config->device_name : DEFAULT_DEVICE_NAME;
    size_t name_len = strlen(device_name);
    if (name_len > MAX_DEVICE_NAME_LEN) {
        name_len = MAX_DEVICE_NAME_LEN;
    }

    /* Update advertising data with device name */
    ad_data[1].data = (const uint8_t *)device_name;
    ad_data[1].data_len = name_len;

    /* Update advertising interval */
    uint16_t interval = config->adv_interval_ms ? config->adv_interval_ms : DEFAULT_ADV_INTERVAL_MS;
    update_advertising_interval(interval);

    /* Configure advertising options */
    if (config->connectable) {
        adv_param.options |= BT_LE_ADV_OPT_CONNECTABLE;
    } else {
        adv_param.options &= ~BT_LE_ADV_OPT_CONNECTABLE;
    }

    /* Initialize connection tracking */
    for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
        current_connections[i] = NULL;
    }
    connection_count = 0;

    /* Enable Bluetooth */
    err = bt_enable(bt_ready_callback);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return BLE_INIT_STATUS_STACK_FAILED;
    }

    ble_initialized = true;
    current_state = BLE_ADVERTISING;

    LOG_INF("BLE initialization started");
    return BLE_INIT_STATUS_SUCCESS;
}

int ble_advertising_start(void)
{
    int err;

    if (!ble_initialized) {
        LOG_ERR("BLE not initialized");
        return -EACCES;
    }

    /* Use NUS service data only if NUS is enabled */
    struct bt_data *scan_data = stored_config.enable_nus ? sd_data : NULL;
    size_t scan_data_len = stored_config.enable_nus ? ARRAY_SIZE(sd_data) : 0;

    err = bt_le_adv_start(&adv_param, ad_data, ARRAY_SIZE(ad_data), scan_data, scan_data_len);
    if (err) {
        LOG_ERR("Advertising failed to start (err %d)", err);
        return err;
    }

    if (connection_count == 0) {
        current_state = BLE_ADVERTISING;
    }

    LOG_INF("Advertising started");
    return 0;
}

int ble_advertising_stop(void)
{
    int err;

    if (!ble_initialized) {
        LOG_ERR("BLE not initialized");
        return -EACCES;
    }

    err = bt_le_adv_stop();
    if (err) {
        LOG_ERR("Advertising failed to stop (err %d)", err);
        return err;
    }

    if (connection_count == 0) {
        current_state = BLE_DISCONNECTED;
    }

    LOG_INF("Advertising stopped");
    return 0;
}

int ble_nus_send(struct bt_conn *conn, const uint8_t *data, uint16_t len)
{
    if (!ble_initialized) {
        LOG_ERR("BLE not initialized");
        return -EACCES;
    }

    if (!stored_config.enable_nus) {
        LOG_ERR("NUS not enabled");
        return -ENOTSUP;
    }

    return bt_nus_send(conn, data, len);
}

enum ble_connection_state ble_get_connection_state(void)
{
    return current_state;
}

uint8_t ble_get_connection_count(void)
{
    return connection_count;
}

int ble_disconnect(struct bt_conn *conn)
{
    if (!ble_initialized) {
        LOG_ERR("BLE not initialized");
        return -EACCES;
    }

    if (!conn) {
        LOG_ERR("Invalid connection");
        return -EINVAL;
    }

    return bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

int ble_disconnect_all(void)
{
    int err = 0;

    if (!ble_initialized) {
        LOG_ERR("BLE not initialized");
        return -EACCES;
    }

    for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
        if (current_connections[i] != NULL) {
            int disconnect_err = bt_conn_disconnect(current_connections[i], 
                                                   BT_HCI_ERR_REMOTE_USER_TERM_CONN);
            if (disconnect_err) {
                LOG_ERR("Failed to disconnect connection %d (err %d)", i, disconnect_err);
                err = disconnect_err;
            }
        }
    }

    return err;
}