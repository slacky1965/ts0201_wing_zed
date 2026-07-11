#include "app_main.h"

config_t config;

/*********************************************************************
 * @fn      checksum
 *
 * @brief   Calculate CRC8 checksum of the given data buffer
 *
 * @param   data - pointer to the data buffer
 * @param   length - number of bytes to process
 *
 * @return  8-bit checksum value
 */
static uint8_t checksum(uint8_t *data, uint16_t length) {

    uint8_t crc8 = 0;

    for(uint16_t i = 0; i < length; i++) {
        crc8 += data[i];
    }

    return crc8;
}


/*********************************************************************
 * @fn      config_save
 *
 * @brief   Save the current configuration to non-volatile storage
 *
 * @param   None
 *
 * @return  nv_sts_t - NV_SUCC on success, error status otherwise
 */
nv_sts_t config_save(void) {
    nv_sts_t st = NV_SUCC;

#if NV_ENABLE

    APP_DEBUG(DEBUG_SAVE_EN, "Saved config\r\n");

    config.crc = checksum((uint8_t*)&config, sizeof(config_t)-1);
    st = nv_flashWriteNew(1, NV_MODULE_APP,  NV_ITEM_APP_USER_CFG, sizeof(config_t), (uint8_t*)&config);

#else
    st = NV_ENABLE_PROTECT_ERROR;
#endif

    return st;
}

/*********************************************************************
 * @fn      config_restore
 *
 * @brief   Restore configuration from non-volatile storage or apply defaults
 *
 * @param   None
 *
 * @return  nv_sts_t - NV_SUCC on success, error status otherwise
 */
nv_sts_t config_restore(void) {
    nv_sts_t st = NV_SUCC;

#if NV_ENABLE

    config_t temp_config;

    st = nv_flashReadNew(1, NV_MODULE_APP,  NV_ITEM_APP_USER_CFG, sizeof(config_t), (uint8_t*)&temp_config);

    if (st == NV_SUCC && temp_config.crc == checksum((uint8_t*)&temp_config, sizeof(config_t)-1)) {

        APP_DEBUG(DEBUG_SAVE_EN, "Restored config\r\n");

        memcpy(&config, &temp_config, (sizeof(config_t)));
    } else {
        /* default config */
        APP_DEBUG(DEBUG_SAVE_EN, "Default config\r\n");
        config.read_sensors_period = DEFAULT_READ_SENSORS_PERIOD;
        config.temperature_offset = DEFAULT_TEMPERATURE_OFFSET;
        config.temperature_onoff = DEFAULT_TEMPERATURE_ONOFF;
        config.temperature_onoff_high = DEFAULT_TEMPERATURE_ONOFF_MAX;
        config.temperature_onoff_low = DEFAULT_TEMPERATURE_ONOFF_MIN;
        config.repeat_cmd = REPEAT_COMMAND_OFF;
        config.humidity_offset = DEFAULT_HUMIDITY_OFFSET;
        config.humidity_onoff = DEFAULT_HUMIDITY_ONOFF;
        config.humidity_onoff_high = DEFAULT_HUMIDITY_ONOFF_MAX;
        config.humidity_onoff_low = DEFAULT_HUMIDITY_ONOFF_MIN;
    }

    g_zcl_temperatureAttrs.read_sensors_period = config.read_sensors_period;
    g_zcl_temperatureAttrs.temperature_offset = config.temperature_offset;
    g_zcl_temperatureAttrs.temperature_onoff = config.temperature_onoff;
    g_zcl_temperatureAttrs.temperature_onoff_high = config.temperature_onoff_high;
    g_zcl_temperatureAttrs.temperature_onoff_low = config.temperature_onoff_low;
    g_zcl_temperatureAttrs.repeat_cmd = config.repeat_cmd;
    g_zcl_humidityAttrs.humidity_offset = config.humidity_offset;
    g_zcl_humidityAttrs.humidity_onoff = config.humidity_onoff;
    g_zcl_humidityAttrs.humidity_onoff_high = config.humidity_onoff_high;
    g_zcl_humidityAttrs.humidity_onoff_low = config.humidity_onoff_low;

#else
    st = NV_ENABLE_PROTECT_ERROR;
#endif

    return st;
}

