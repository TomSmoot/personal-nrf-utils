/*
 * Copyright (c) 2025 Personal NRF Utils
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nrf_utils.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/pm/pm.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#ifdef CONFIG_HEAP_MEM_POOL_SIZE
#include <zephyr/sys/heap_listener.h>
#endif

LOG_MODULE_REGISTER(nrf_utils, LOG_LEVEL_DBG);

/* Battery monitoring via ADC */
#if DT_NODE_EXISTS(DT_PATH(zephyr_user)) && DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#define ADC_NODE DT_PATH(zephyr_user)
#else
#define ADC_NODE DT_CHILD(DT_PATH(soc), adc_14000000)
#endif

#if DT_NODE_EXISTS(ADC_NODE)
static const struct device *adc_dev = DEVICE_DT_GET(ADC_NODE);
static struct adc_channel_cfg adc_cfg = {
    .gain = ADC_GAIN_1_6,
    .reference = ADC_REF_INTERNAL,
    .acquisition_time = ADC_ACQ_TIME_DEFAULT,
    .channel_id = 0,
    .differential = 0,
#ifdef CONFIG_ADC_NRFX_SAADC
    .input_positive = SAADC_CH_PSELP_PSELP_VDD,
#endif
};
#endif

/* Temperature sensor */
#if DT_NODE_EXISTS(DT_NODELABEL(temp))
static const struct device *temp_dev = DEVICE_DT_GET(DT_NODELABEL(temp));
#elif DT_NODE_EXISTS(DT_INST(0, nordic_nrf_temp))
static const struct device *temp_dev = DEVICE_DT_GET(DT_INST(0, nordic_nrf_temp));
#endif

static bool utils_initialized = false;

int nrf_utils_init(void)
{
    int err;

    if (utils_initialized) {
        return 0;
    }

    LOG_INF("Initializing nRF utilities");

#if DT_NODE_EXISTS(ADC_NODE)
    /* Initialize ADC for battery monitoring */
    if (!device_is_ready(adc_dev)) {
        LOG_ERR("ADC device not ready");
        return -ENODEV;
    }

    err = adc_channel_setup(adc_dev, &adc_cfg);
    if (err < 0) {
        LOG_ERR("Failed to setup ADC channel (err %d)", err);
        return err;
    }
#endif

#if defined(temp_dev)
    /* Initialize temperature sensor */
    if (temp_dev && !device_is_ready(temp_dev)) {
        LOG_WRN("Temperature sensor not ready");
    }
#endif

    utils_initialized = true;
    LOG_INF("nRF utilities initialized successfully");
    return 0;
}

int nrf_get_battery_voltage_mv(void)
{
#if DT_NODE_EXISTS(ADC_NODE)
    int16_t sample_buffer;
    int err;

    struct adc_sequence sequence = {
        .channels = BIT(adc_cfg.channel_id),
        .buffer = &sample_buffer,
        .buffer_size = sizeof(sample_buffer),
        .resolution = 12,
    };

    if (!device_is_ready(adc_dev)) {
        return -ENODEV;
    }

    err = adc_read(adc_dev, &sequence);
    if (err < 0) {
        LOG_ERR("ADC read failed (err %d)", err);
        return err;
    }

    /* Convert ADC value to millivolts */
    /* For nRF52/nRF53: VDD measurement with 1/6 gain and internal reference (0.6V) */
    /* ADC_value = VDD * (1/6) / 0.6V * 4095 */
    /* VDD = ADC_value * 0.6V * 6 / 4095 */
    int32_t voltage_mv = ((int32_t)sample_buffer * 600 * 6) / 4095;

    LOG_DBG("Battery voltage: %d mV (ADC: %d)", voltage_mv, sample_buffer);
    return voltage_mv;
#else
    LOG_WRN("Battery voltage monitoring not available");
    return -ENOTSUP;
#endif
}

int nrf_get_battery_percentage(void)
{
    int voltage_mv = nrf_get_battery_voltage_mv();
    if (voltage_mv < 0) {
        return voltage_mv;
    }

    /* Simple linear approximation for Li-ion battery */
    /* 3000mV = 0%, 4200mV = 100% */
    if (voltage_mv <= 3000) {
        return 0;
    } else if (voltage_mv >= 4200) {
        return 100;
    } else {
        return ((voltage_mv - 3000) * 100) / (4200 - 3000);
    }
}

int nrf_get_battery_status(struct nrf_battery_status *status)
{
    if (!status) {
        return -EINVAL;
    }

    int voltage = nrf_get_battery_voltage_mv();
    if (voltage < 0) {
        return voltage;
    }

    status->voltage_mv = voltage;
    status->percentage = nrf_get_battery_percentage();
    status->is_present = (voltage > 1000); /* Assume battery present if >1V */
    status->is_charging = false; /* Would need charge detection circuit */

    return 0;
}

int nrf_get_temperature_celsius(void)
{
#if defined(temp_dev)
    struct sensor_value temp_val;
    int err;

    if (!temp_dev || !device_is_ready(temp_dev)) {
        return -ENODEV;
    }

    err = sensor_sample_fetch(temp_dev);
    if (err < 0) {
        LOG_ERR("Failed to fetch temperature sample (err %d)", err);
        return err;
    }

    err = sensor_channel_get(temp_dev, SENSOR_CHAN_DIE_TEMP, &temp_val);
    if (err < 0) {
        LOG_ERR("Failed to get temperature value (err %d)", err);
        return err;
    }

    /* Convert to integer Celsius */
    int temp_celsius = sensor_value_to_double(&temp_val);
    LOG_DBG("Temperature: %d°C", temp_celsius);
    return temp_celsius;
#else
    LOG_WRN("Temperature sensor not available");
    return -ENOTSUP;
#endif
}

uint32_t nrf_get_uptime_ms(void)
{
    return k_uptime_get_32();
}

uint32_t nrf_get_free_heap_bytes(void)
{
#ifdef CONFIG_HEAP_MEM_POOL_SIZE
    struct sys_memory_stats stats;
    sys_heap_runtime_stats_get(&_system_heap, &stats);
    return stats.free_bytes;
#else
    LOG_WRN("Heap tracking not enabled");
    return 0;
#endif
}

int nrf_get_system_info(struct nrf_system_info *info)
{
    if (!info) {
        return -EINVAL;
    }

    info->board_name = CONFIG_BOARD;
    info->soc_name = CONFIG_SOC;
    info->uptime_ms = nrf_get_uptime_ms();
    info->free_heap_bytes = nrf_get_free_heap_bytes();
    info->reset_reason = 0; /* Would need reset reason detection */

    return 0;
}

void nrf_system_reset(void)
{
    LOG_INF("System reset requested");
    sys_reboot(SYS_REBOOT_COLD);
}

void nrf_deep_sleep(uint32_t duration_ms)
{
    LOG_INF("Entering deep sleep for %u ms", duration_ms);
    
    if (duration_ms > 0) {
        k_sleep(K_MSEC(duration_ms));
    } else {
        /* Enter indefinite sleep - would need wake sources configured */
        k_sleep(K_FOREVER);
    }
}
