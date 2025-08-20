/*
 * Copyright (c) 2025 Personal NRF Utils
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CMD_PARSER_H_
#define CMD_PARSER_H_

/**
 * @file
 * @brief Simple command line parser for BLE serial interface
 */

#include <zephyr/types.h>
#include <zephyr/bluetooth/conn.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Maximum command length */
#define CMD_MAX_LEN 128

/** @brief Maximum response length */
#define CMD_RESPONSE_MAX_LEN 256

/** @brief Command handler function type */
typedef int (*cmd_handler_t)(const char *args, char *response, size_t response_size);

/** @brief Command structure */
struct cmd_entry {
    const char *name;
    const char *help;
    cmd_handler_t handler;
};

/**
 * @brief Initialize command parser
 *
 * @return 0 on success, negative error code otherwise
 */
int cmd_parser_init(void);

/**
 * @brief Process received data as commands
 *
 * @param conn BLE connection (for sending responses)
 * @param data Received data
 * @param len Data length
 *
 * @return 0 on success, negative error code otherwise
 */
int cmd_parser_process(struct bt_conn *conn, const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* CMD_PARSER_H_ */
