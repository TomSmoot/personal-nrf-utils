#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>

#include "modules/ble_common/ble_init.h"
#include "modules/nrf_utils/nrf_utils.h"
#include "modules/cmd_parser/cmd_parser.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* LED configuration */
#define LED0_NODE DT_ALIAS(led0)
const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/* Application state */
static bool auto_status_enabled = true;

/* BLE event callbacks */
static void on_ble_ready(void)
{
    LOG_INF("BLE stack initialized and ready");
    gpio_pin_set_dt(&led, 1);
    
    /* Send welcome message after a short delay */
    k_sleep(K_MSEC(500));
    char welcome[] = "\n=== nRF Utils Test Device ===\n"
                    "Type 'help' for available commands\n"
                    "Auto status updates: enabled\n\n";
    
    if (ble_get_connection_state() == BLE_CONNECTED) {
        ble_nus_send(NULL, (uint8_t *)welcome, strlen(welcome));
    }
}

static void on_connected(struct bt_conn *conn, uint8_t conn_err)
{
    if (conn_err) {
        LOG_ERR("Connection failed (err %u)", conn_err);
        return;
    }
    
    LOG_INF("Device connected");
    gpio_pin_toggle_dt(&led);
    
    /* Send welcome message */
    k_sleep(K_MSEC(1000)); /* Give client time to setup notifications */
    char welcome[] = "\n=== nRF Utils Test Device Connected ===\n"
                    "Type 'help' for available commands\n\n";
    ble_nus_send(conn, (uint8_t *)welcome, strlen(welcome));
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("Device disconnected (reason %u)", reason);
    gpio_pin_set_dt(&led, 1);
}

static void on_data_received(struct bt_conn *conn, const uint8_t *data, uint16_t len)
{
    LOG_INF("Received %u bytes via NUS", len);
    LOG_HEXDUMP_DBG(data, len, "RX Data:");
    
    /* Process as commands */
    cmd_parser_process(conn, data, len);
}

/* BLE configuration */
static const struct ble_init_config ble_config = {
    .device_name = "nRF_Utils_Device",
    .adv_interval_ms = 100,
    .connectable = true,
    .enable_nus = true,
};

static const struct ble_event_callbacks ble_callbacks = {
    .ready = on_ble_ready,
    .connected = on_connected,
    .disconnected = on_disconnected,
    .nus_data_received = on_data_received,
};

int main(void)
{
    int err;
    uint32_t counter = 0;
    char status_msg[256];

    LOG_INF("Starting nRF Utils Test Application");

    /* Initialize LED */
    if (!gpio_is_ready_dt(&led)) {
        LOG_ERR("LED device not ready");
        return -ENODEV;
    }

    err = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    if (err < 0) {
        LOG_ERR("Failed to configure LED pin");
        return err;
    }

    /* Initialize utilities */
    err = nrf_utils_init();
    if (err) {
        LOG_ERR("nRF utilities initialization failed (err %d)", err);
        return err;
    }

    /* Initialize command parser */
    err = cmd_parser_init();
    if (err) {
        LOG_ERR("Command parser initialization failed (err %d)", err);
        return err;
    }

    /* Initialize BLE */
    err = ble_init(&ble_config, &ble_callbacks);
    if (err) {
        LOG_ERR("BLE initialization failed (err %d)", err);
        return err;
    }

    LOG_INF("All systems initialized, entering main loop");

    /* Main application loop */
    while (1) {
        k_sleep(K_SECONDS(10)); /* Status update every 10 seconds */
        
        /* Send periodic status if connected and auto status enabled */
        if (auto_status_enabled && ble_get_connection_state() == BLE_CONNECTED) {
            struct nrf_battery_status battery;
            int temp = nrf_get_temperature_celsius();
            uint32_t uptime = nrf_get_uptime_ms();
            
            int pos = 0;
            pos += snprintf(status_msg + pos, sizeof(status_msg) - pos, 
                           "[AUTO] Uptime: %u.%03us", uptime / 1000, uptime % 1000);
            
            if (nrf_get_battery_status(&battery) == 0) {
                pos += snprintf(status_msg + pos, sizeof(status_msg) - pos,
                               " | Battery: %u%% (%umV)", battery.percentage, battery.voltage_mv);
            }
            
            if (temp >= -100) {
                pos += snprintf(status_msg + pos, sizeof(status_msg) - pos,
                               " | Temp: %d°C", temp);
            }
            
            pos += snprintf(status_msg + pos, sizeof(status_msg) - pos, "\n");
            
            int ret = ble_nus_send(NULL, (uint8_t *)status_msg, strlen(status_msg));
            if (ret == 0) {
                LOG_DBG("Sent auto status update");
            } else {
                LOG_WRN("Failed to send auto status (err %d)", ret);
            }
        }
        
        counter++;
        LOG_DBG("Main loop heartbeat #%u", counter);
    }

    return 0;
}