#include "app_main.h"

static int16_t raw_temp_zcl;
static uint16_t raw_hum_zcl;
static int16_t sensor_temperature;
static int16_t sensor_humidity;

/*********************************************************************
 * @fn      sensor_update_zcl
 *
 * @brief   Apply user-configured offsets to raw sensor values and clamp
 *          humidity to the valid range 0-10000 (0.00-100.00%).
 *
 * @param   None
 *
 * @return  None
 */
static void sensor_update_zcl(void) {
    sensor_temperature = raw_temp_zcl + config.temperature_offset;
    sensor_humidity = raw_hum_zcl + config.humidity_offset;
    if (sensor_humidity < 0) sensor_humidity = 0;
    if (sensor_humidity > 10000) sensor_humidity = 10000;
}

/*********************************************************************
 * @fn      app_sensor_measurement
 *
 * @brief   Read temperature and humidity from the SHT30 sensor, apply
 *          calibration offsets, update ZCL attributes, and refresh the LCD.
 *
 * @param   None
 *
 * @return  None
 */
void app_sensor_measurement(void) {
    int16_t temp_zcl;
    uint16_t hum_zcl;

    if (!sht30_read(&temp_zcl, &hum_zcl)) {
        APP_DEBUG(DEBUG_SENSOR_EN, "sht30 read fail\r\n");
        return;
    }

    raw_temp_zcl = temp_zcl;
    raw_hum_zcl = hum_zcl;
    sensor_update_zcl();

    APP_DEBUG(DEBUG_SENSOR_EN, "T=%d H=%d\r\n", sensor_temperature, sensor_humidity);

    zcl_setAttrVal(APP_ENDPOINT1, ZCL_CLUSTER_MS_TEMPERATURE_MEASUREMENT,
                    ZCL_TEMPERATURE_MEASUREMENT_ATTRID_MEASUREDVALUE,
                    (uint8_t*)&sensor_temperature);

    zcl_setAttrVal(APP_ENDPOINT1, ZCL_CLUSTER_MS_RELATIVE_HUMIDITY,
                    ZCL_RELATIVE_HUMIDITY_MEASUREMENT_ATTRID_MEASUREDVALUE,
                    (uint8_t*)&sensor_humidity);

    lcd_update_temp_hum(sensor_temperature / 10, sensor_humidity / 100);
}

/*********************************************************************
 * @fn      app_sensor_set_temperature
 *
 * @brief   Recalculate temperature with offsets, update the ZCL attribute,
 *          and refresh the LCD display.
 *
 * @param   None
 *
 * @return  None
 */
void app_sensor_set_temperature(void) {
    sensor_update_zcl();
    zcl_setAttrVal(APP_ENDPOINT1, ZCL_CLUSTER_MS_TEMPERATURE_MEASUREMENT,
                    ZCL_TEMPERATURE_MEASUREMENT_ATTRID_MEASUREDVALUE,
                    (uint8_t*)&sensor_temperature);
    lcd_update_temp_hum(sensor_temperature / 10, sensor_humidity / 100);
}

/*********************************************************************
 * @fn      app_sensor_set_humidity
 *
 * @brief   Recalculate humidity with offsets, update the ZCL attribute,
 *          and refresh the LCD display.
 *
 * @param   None
 *
 * @return  None
 */
void app_sensor_set_humidity(void) {
    sensor_update_zcl();
    zcl_setAttrVal(APP_ENDPOINT1, ZCL_CLUSTER_MS_RELATIVE_HUMIDITY,
                    ZCL_RELATIVE_HUMIDITY_MEASUREMENT_ATTRID_MEASUREDVALUE,
                    (uint8_t*)&sensor_humidity);
    lcd_update_temp_hum(sensor_temperature / 10, sensor_humidity / 100);
}

/*********************************************************************
 * @fn      app_sensor_get_temperature
 *
 * @brief   Get the current calibrated temperature value in ZCL format.
 *
 * @param   None
 *
 * @return  Temperature in 0.01C units
 */
int16_t app_sensor_get_temperature(void) {
    return sensor_temperature;
}

/*********************************************************************
 * @fn      app_sensor_get_humidity
 *
 * @brief   Get the current calibrated humidity value in ZCL format.
 *
 * @param   None
 *
 * @return  Humidity in 0.01% units
 */
int16_t app_sensor_get_humidity(void) {
    return sensor_humidity;
}

/*********************************************************************
 * @fn      app_sensor_get_period
 *
 * @brief   Calculate the sensor polling period based on active ZCL reporting
 *          entries for temperature and humidity clusters, and update the
 *          application context polling rate accordingly.
 *
 * @param   None
 *
 * @return  The calculated sensor read period in seconds
 */
uint16_t app_sensor_get_period(void) {
    uint16_t period, p_temp = 0, p_hum = 0;

    if (zcl_reportingEntryActiveNumGet()) {
        for (uint8_t i = 0; i < ZCL_REPORTING_TABLE_NUM; i++) {
            reportCfgInfo_t *pEntry = &reportingTab.reportCfgInfo[i];
            if (pEntry->used && zb_bindingTblSearched(pEntry->clusterID, pEntry->endPoint)) {
                if (pEntry->clusterID == ZCL_CLUSTER_MS_TEMPERATURE_MEASUREMENT) {
                    if (pEntry->maxInterval == 0xFFFF || pEntry->minInterval == 0) {
                        p_temp = pEntry->maxInterval;
                    } else if (pEntry->minInterval != 0xFFFF && pEntry->maxInterval == 0) {
                        p_temp = 0;
                    } else {
                        p_temp = pEntry->minInterval;
                    }
                } else if (pEntry->clusterID == ZCL_CLUSTER_MS_RELATIVE_HUMIDITY) {
                    if (pEntry->maxInterval == 0xFFFF || pEntry->minInterval == 0) {
                        p_hum = pEntry->maxInterval;
                    } else if (pEntry->minInterval != 0xFFFF && pEntry->maxInterval == 0) {
                        p_hum = 0;
                    } else {
                        p_hum = pEntry->minInterval;
                    }
                }
            }
        }
    }

    if ((!p_temp && !p_hum) || (p_temp == 0xFFFF && p_hum == 0xFFFF)) period = 0;
    else if (p_temp == 0xFFFF) period = p_hum;
    else if (p_hum == 0xFFFF) period = p_temp;
    else period = (p_temp < p_hum) ? p_temp : p_hum;

    if (!period || period > READ_SENSORS_PERIOD_MAX)
        g_appCtx.read_sensor_period = config.read_sensors_period;
    else
        g_appCtx.read_sensor_period = period;

    g_appCtx.read_sensor_period95p = (g_appCtx.read_sensor_period * 1000) / 100 * 95;

    if (!g_appCtx.timerSetPollRateEvt) app_setPollRate(TIMEOUT_3SEC);

    return g_appCtx.read_sensor_period;
}

