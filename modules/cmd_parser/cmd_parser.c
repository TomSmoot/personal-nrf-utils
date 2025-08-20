/*
 * Copyright (c) 2025 Personal NRF Utils
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cmd_parser.h"
#include "../ble_common/ble_init.h"
#include "../nrf_utils/nrf_utils.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(cmd_parser, LOG_LEVEL_DBG);

/* Command buffer */
static char cmd_buffer[CMD_MAX_LEN];
static size_t cmd_buffer_pos = 0;

/* Forward declarations */
static int cmd_help(const char *args, char *response, size_t response_size);
static int cmd_status(const char *args, char *response, size_t response_size);
static int cmd_battery(const char *args, char *response, size_t response_size);
static int cmd_temp(const char *args, char *response, size_t response_size);
static int cmd_info(const char *args, char *response, size_t response_size);
static int cmd_uptime(const char *args, char *response, size_t response_size);
static int cmd_reset(const char *args, char *response, size_t response_size);
static int cmd_led(const char *args, char *response, size_t response_size);
static int cmd_echo(const char *args, char *response, size_t response_size);

/* Command table */
static const struct cmd_entry commands[] = {
    {"help", "Show available commands", cmd_help},
    {"status", "Show system status", cmd_status},
    {"battery", "Show battery status", cmd_battery},
    {"temp", "Show temperature", cmd_temp},
    {"info", "Show system information", cmd_info},
    {"uptime", "Show system uptime", cmd_uptime},
    {"reset", "Reset the system", cmd_reset},
    {"led", "Control LED (on|off|toggle)", cmd_led},
    {"echo", "Echo back the arguments", cmd_echo},
};

#define NUM_COMMANDS (sizeof(commands) / sizeof(commands[0]))

/* External LED reference */
extern const struct gpio_dt_spec led;

int cmd_parser_init(void)
{
    LOG_INF("Command parser initialized");
    return 0;
}

static int cmd_help(const char *args, char *response, size_t response_size)
{
    int pos = 0;
    
    pos += snprintf(response + pos, response_size - pos, "Available commands:\n");
    
    for (size_t i = 0; i < NUM_COMMANDS && pos < response_size - 50; i++) {
        pos += snprintf(response + pos, response_size - pos, "  %s - %s\n",
                       commands[i].name, commands[i].help);
    }
    
    pos += snprintf(response + pos, response_size - pos, "\nType 'command help' for usage\n");
    
    return 0;
}

static int cmd_status(const char *args, char *response, size_t response_size)
{
    struct nrf_battery_status battery;
    int temp = nrf_get_temperature_celsius();
    uint32_t uptime = nrf_get_uptime_ms();
    uint8_t conn_count = ble_get_connection_count();
    
    int pos = 0;
    pos += snprintf(response + pos, response_size - pos, "=== System Status ===\n");
    pos += snprintf(response + pos, response_size - pos, "Uptime: %u.%03u seconds\n", 
                   uptime / 1000, uptime % 1000);
    pos += snprintf(response + pos, response_size - pos, "BLE connections: %u\n", conn_count);
    
    if (nrf_get_battery_status(&battery) == 0) {
        pos += snprintf(response + pos, response_size - pos, "Battery: %u%% (%u mV)\n",
                       battery.percentage, battery.voltage_mv);
    }
    
    if (temp >= 0) {
        pos += snprintf(response + pos, response_size - pos, "Temperature: %d°C\n", temp);
    }
    
    return 0;
}

static int cmd_battery(const char *args, char *response, size_t response_size)
{
    struct nrf_battery_status battery;
    int ret = nrf_get_battery_status(&battery);
    
    if (ret < 0) {
        snprintf(response, response_size, "Battery status unavailable (err %d)\n", ret);
        return ret;
    }
    
    snprintf(response, response_size, "Battery Status:\n"
             "  Voltage: %u mV\n"
             "  Percentage: %u%%\n"
             "  Present: %s\n"
             "  Charging: %s\n",
             battery.voltage_mv,
             battery.percentage,
             battery.is_present ? "Yes" : "No",
             battery.is_charging ? "Yes" : "No");
    
    return 0;
}

static int cmd_temp(const char *args, char *response, size_t response_size)
{
    int temp = nrf_get_temperature_celsius();
    
    if (temp < -100) {
        snprintf(response, response_size, "Temperature unavailable (err %d)\n", temp);
        return temp;
    }
    
    snprintf(response, response_size, "Temperature: %d°C\n", temp);
    return 0;
}

static int cmd_info(const char *args, char *response, size_t response_size)
{
    struct nrf_system_info info;
    int ret = nrf_get_system_info(&info);
    
    if (ret < 0) {
        snprintf(response, response_size, "System info unavailable (err %d)\n", ret);
        return ret;
    }
    
    snprintf(response, response_size, "System Information:\n"
             "  Board: %s\n"
             "  SoC: %s\n"
             "  Uptime: %u ms\n"
             "  Free Heap: %u bytes\n",
             info.board_name,
             info.soc_name,
             info.uptime_ms,
             info.free_heap_bytes);
    
    return 0;
}

static int cmd_uptime(const char *args, char *response, size_t response_size)
{
    uint32_t uptime = nrf_get_uptime_ms();
    uint32_t seconds = uptime / 1000;
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;
    
    snprintf(response, response_size, "Uptime: %u hours, %u minutes, %u seconds\n",
             hours, minutes % 60, seconds % 60);
    
    return 0;
}

static int cmd_reset(const char *args, char *response, size_t response_size)
{
    snprintf(response, response_size, "Resetting system in 2 seconds...\n");
    
    /* Send response first, then reset */
    k_sleep(K_MSEC(100)); /* Give time for BLE transmission */
    nrf_system_reset();
    
    return 0;
}

static int cmd_led(const char *args, char *response, size_t response_size)
{
    if (!args || strlen(args) == 0) {
        snprintf(response, response_size, "Usage: led <on|off|toggle>\n");
        return -EINVAL;
    }
    
    if (strncmp(args, "on", 2) == 0) {
        gpio_pin_set_dt(&led, 1);
        snprintf(response, response_size, "LED turned on\n");
    } else if (strncmp(args, "off", 3) == 0) {
        gpio_pin_set_dt(&led, 0);
        snprintf(response, response_size, "LED turned off\n");
    } else if (strncmp(args, "toggle", 6) == 0) {
        gpio_pin_toggle_dt(&led);
        snprintf(response, response_size, "LED toggled\n");
    } else {
        snprintf(response, response_size, "Invalid LED command. Use: on, off, or toggle\n");
        return -EINVAL;
    }
    
    return 0;
}

static int cmd_echo(const char *args, char *response, size_t response_size)
{
    if (!args || strlen(args) == 0) {
        snprintf(response, response_size, "Echo: (no arguments)\n");
    } else {
        snprintf(response, response_size, "Echo: %s\n", args);
    }
    
    return 0;
}

static int execute_command(const char *cmd_line, char *response, size_t response_size)
{
    char *cmd_name;
    char *args;
    char *cmd_copy;
    
    /* Make a copy for parsing */
    cmd_copy = k_malloc(strlen(cmd_line) + 1);
    if (!cmd_copy) {
        snprintf(response, response_size, "Out of memory\n");
        return -ENOMEM;
    }
    
    strcpy(cmd_copy, cmd_line);
    
    /* Parse command name and arguments */
    cmd_name = strtok(cmd_copy, " \t");
    args = strtok(NULL, "");
    
    if (!cmd_name) {
        k_free(cmd_copy);
        return 0; /* Empty command */
    }
    
    /* Find and execute command */
    for (size_t i = 0; i < NUM_COMMANDS; i++) {
        if (strcmp(cmd_name, commands[i].name) == 0) {
            int ret = commands[i].handler(args, response, response_size);
            k_free(cmd_copy);
            return ret;
        }
    }
    
    /* Command not found */
    snprintf(response, response_size, "Unknown command: %s\nType 'help' for available commands\n", cmd_name);
    k_free(cmd_copy);
    return -ENOENT;
}

int cmd_parser_process(struct bt_conn *conn, const uint8_t *data, uint16_t len)
{
    char response[CMD_RESPONSE_MAX_LEN];
    
    /* Add received data to buffer */
    for (uint16_t i = 0; i < len; i++) {
        char c = data[i];
        
        if (c == '\n' || c == '\r') {
            /* End of command */
            if (cmd_buffer_pos > 0) {
                cmd_buffer[cmd_buffer_pos] = '\0';
                
                LOG_INF("Processing command: %s", cmd_buffer);
                
                /* Execute command */
                int ret = execute_command(cmd_buffer, response, sizeof(response));
                
                /* Send response */
                if (strlen(response) > 0) {
                    ble_nus_send(conn, (uint8_t *)response, strlen(response));
                }
                
                /* Reset buffer */
                cmd_buffer_pos = 0;
            }
        } else if (c >= 32 && c <= 126) { /* Printable characters */
            if (cmd_buffer_pos < CMD_MAX_LEN - 1) {
                cmd_buffer[cmd_buffer_pos++] = c;
            }
        } else if (c == '\b' || c == 127) { /* Backspace */
            if (cmd_buffer_pos > 0) {
                cmd_buffer_pos--;
            }
        }
    }
    
    return 0;
}
