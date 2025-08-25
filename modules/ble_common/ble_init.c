/*
 * Copyright (c) 2025 Personal NRF Utils
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ble_init.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

LOG_MODULE_REGISTER(ble_init, LOG_LEVEL_DBG);

/* Default device name if none provided */
#define DEFAULT_DEVICE_NAME "nRF5340_Device"

/* IPC communication setup */
#define IPC_SERVICE_NAME "nrf5340_ble_ipc"
static const struct device *ipc_instance;
static struct ipc_ept ble_endpoint;

/* Module state */
static bool ble_initialized = false;
static enum ble_connection_state current_state = BLE_DISCONNECTED;
static struct ble_init_config stored_config;
static const struct ble_event_callbacks *event_callbacks = NULL;
static bool ipc_ready = false;

/* Message types for IPC communication */
enum ipc_msg_type {
    IPC_MSG_INIT = 1,
    IPC_MSG_SEND_DATA = 2,
    IPC_MSG_CONNECTION_STATE = 3,
    IPC_MSG_DATA_RECEIVED = 4,
    IPC_MSG_TEST = 5,
};

/* IPC message structure */
struct ipc_message {
    uint8_t type;
    uint8_t data_len;
    uint8_t data[128];  /* Adjust size as needed */
} __packed;

/* Forward declarations */
static void ipc_endpoint_bound(void *priv);
static void ipc_endpoint_received(const void *data, size_t len, void *priv);
static int send_ipc_message(enum ipc_msg_type type, const uint8_t *data, uint8_t len);

/* IPC endpoint configuration */
static struct ipc_ept_cfg ble_ept_cfg = {
    .name = "ble_endpoint",
    .cb = {
        .bound = ipc_endpoint_bound,
        .received = ipc_endpoint_received,
    },
};

static void ipc_endpoint_bound(void *priv)
{
    LOG_INF("BLE IPC endpoint bound");
    ipc_ready = true;
    
    /* Send initialization message to network core */
    if (ble_initialized) {
        struct ipc_message init_msg = {
            .type = IPC_MSG_INIT,
            .data_len = 0,
        };
        
        /* Add device name to init message */
        const char *name = stored_config.device_name ? stored_config.device_name : DEFAULT_DEVICE_NAME;
        size_t name_len = strlen(name);
        if (name_len < sizeof(init_msg.data)) {
            init_msg.data_len = name_len;
            memcpy(init_msg.data, name, name_len);
        }
        
        int ret = ipc_service_send(&ble_endpoint, &init_msg, sizeof(init_msg));
        if (ret < 0) {
            LOG_ERR("Failed to send init message (err %d)", ret);
        } else {
            LOG_INF("Sent BLE init message to network core");
            current_state = BLE_ADVERTISING;
            
            /* Call ready callback */
            if (event_callbacks && event_callbacks->ready) {
                event_callbacks->ready();
            }
        }
    }
}

static void ipc_endpoint_received(const void *data, size_t len, void *priv)
{
    const struct ipc_message *msg = (const struct ipc_message *)data;
    
    LOG_DBG("Received IPC message type %d, len %d", msg->type, msg->data_len);
    
    switch (msg->type) {
    case IPC_MSG_CONNECTION_STATE:
        if (msg->data_len >= 1) {
            enum ble_connection_state new_state = (enum ble_connection_state)msg->data[0];
            
            if (new_state != current_state) {
                enum ble_connection_state old_state = current_state;
                current_state = new_state;
                
                LOG_INF("BLE state changed: %d -> %d", old_state, new_state);
                
                /* Trigger appropriate callbacks */
                if (event_callbacks) {
                    if (new_state == BLE_CONNECTED && old_state != BLE_CONNECTED) {
                        if (event_callbacks->connected) {
                            event_callbacks->connected();
                        }
                    } else if (new_state != BLE_CONNECTED && old_state == BLE_CONNECTED) {
                        if (event_callbacks->disconnected) {
                            event_callbacks->disconnected(0); /* Reason unknown via IPC */
                        }
                    }
                }
            }
        }
        break;
        
    case IPC_MSG_DATA_RECEIVED:
        LOG_INF("Received BLE data via IPC: %d bytes", msg->data_len);
        LOG_HEXDUMP_DBG(msg->data, msg->data_len, "BLE Data:");
        
        if (event_callbacks && event_callbacks->data_received) {
            event_callbacks->data_received(msg->data, msg->data_len);
        }
        break;
        
    case IPC_MSG_TEST:
        LOG_INF("Received IPC test response: %.*s", msg->data_len, msg->data);
        break;
        
    default:
        LOG_WRN("Unknown IPC message type: %d", msg->type);
        break;
    }
}

static int send_ipc_message(enum ipc_msg_type type, const uint8_t *data, uint8_t len)
{
    if (!ipc_ready) {
        LOG_ERR("IPC not ready");
        return -ENOTCONN;
    }
    
    if (len > sizeof(((struct ipc_message *)0)->data)) {
        LOG_ERR("Data too large for IPC message");
        return -EINVAL;
    }
    
    struct ipc_message msg = {
        .type = type,
        .data_len = len,
    };
    
    if (data && len > 0) {
        memcpy(msg.data, data, len);
    }
    
    int ret = ipc_service_send(&ble_endpoint, &msg, sizeof(msg));
    if (ret < 0) {
        LOG_ERR("Failed to send IPC message (err %d)", ret);
        return ret;
    }
    
    LOG_DBG("Sent IPC message type %d, len %d", type, len);
    return 0;
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
        return BLE_INIT_STATUS_IPC_FAILED;
    }

    LOG_INF("Initializing BLE for nRF5340 application core");

    /* Store configuration and callbacks */
    stored_config = *config;
    event_callbacks = callbacks;

    /* Get IPC service instance */
    ipc_instance = DEVICE_DT_GET(DT_NODELABEL(ipc0));
    if (!ipc_instance) {
        LOG_ERR("Failed to get IPC service instance");
        return BLE_INIT_STATUS_IPC_FAILED;
    }

    /* Register IPC endpoint */
    err = ipc_service_register_endpoint(ipc_instance, &ble_endpoint, &ble_ept_cfg);
    if (err) {
        LOG_ERR("Failed to register IPC endpoint (err %d)", err);
        return BLE_INIT_STATUS_IPC_FAILED;
    }

    ble_initialized = true;
    current_state = BLE_DISCONNECTED;

    LOG_INF("BLE IPC initialization complete, waiting for network core");
    return BLE_INIT_STATUS_SUCCESS;
}

int ble_send_data(const uint8_t *data, uint16_t len)
{
    if (!ble_initialized) {
        LOG_ERR("BLE not initialized");
        return -EACCES;
    }

    if (!data || len == 0) {
        LOG_ERR("Invalid data parameters");
        return -EINVAL;
    }

    /* Split large messages if needed */
    uint16_t remaining = len;
    uint16_t offset = 0;
    
    while (remaining > 0) {
        uint8_t chunk_size = (remaining > 120) ? 120 : remaining; /* Leave room for header */
        
        int ret = send_ipc_message(IPC_MSG_SEND_DATA, data + offset, chunk_size);
        if (ret < 0) {
            return ret;
        }
        
        offset += chunk_size;
        remaining -= chunk_size;
        
        /* Small delay between chunks to avoid overwhelming the network core */
        if (remaining > 0) {
            k_sleep(K_MSEC(10));
        }
    }

    /* Call data sent callback */
    if (event_callbacks && event_callbacks->data_sent) {
        event_callbacks->data_sent();
    }

    return 0;
}

enum ble_connection_state ble_get_connection_state(void)
{
    if (!ipc_ready) {
        return BLE_IPC_ERROR;
    }
    return current_state;
}

bool ble_is_ipc_ready(void)
{
    return ipc_ready;
}

int ble_test_ipc_communication(void)
{
    if (!ble_initialized) {
        LOG_ERR("BLE not initialized");
        return -EACCES;
    }

    const char *test_msg = "IPC Test from App Core";
    return send_ipc_message(IPC_MSG_TEST, (const uint8_t *)test_msg, strlen(test_msg));
}