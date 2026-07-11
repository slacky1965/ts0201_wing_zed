#include "app_main.h"

#define SHT30_ADDR      0x44
#define SHT30_ADDR_W    ((SHT30_ADDR << 1) | 0)
#define SHT30_ADDR_R    ((SHT30_ADDR << 1) | 1)

/*********************************************************************
 * @fn      crc8
 *
 * @brief   Calculate CRC-8 checksum using polynomial 0x31.
 *
 * @param   data - pointer to data buffer
 * @param   len  - length of data in bytes
 *
 * @return  CRC-8 checksum value
 */
static uint8_t crc8(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0xFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x31;
            else
                crc <<= 1;
        }
    }
    return crc;
}

/*********************************************************************
 * @fn      sht30_measure_and_read
 *
 * @brief   Trigger a single-shot measurement on the SHT30 sensor and read
 *          the raw temperature and humidity data over I2C.
 *
 * @param   temp_raw - pointer to store raw temperature value
 * @param   hum_raw  - pointer to store raw humidity value
 *
 * @return  1 on success, 0 on failure
 */
static uint8_t sht30_measure_and_read(uint16_t *temp_raw, uint16_t *hum_raw) {
    uint8_t cmd[2] = {0x24, 0x00};
    uint8_t data[6];
    uint8_t ack;

    APP_DEBUG(DEBUG_SENSOR_EN, "WR:");
    sw_i2c_start();
    ack = sw_i2c_write_byte(SHT30_ADDR_W);
    APP_DEBUG(DEBUG_SENSOR_EN, "%d", ack);
    for (uint8_t i = 0; i < 2; i++) {
        ack = sw_i2c_write_byte(cmd[i]);
        APP_DEBUG(DEBUG_SENSOR_EN, "%d", ack);
    }
    sw_i2c_stop();
    APP_DEBUG(DEBUG_SENSOR_EN, " OK\r\n");

    sleep_ms(30);

    APP_DEBUG(DEBUG_SENSOR_EN, "RD:");
    sw_i2c_start();
    ack = sw_i2c_write_byte(SHT30_ADDR_R);
    APP_DEBUG(DEBUG_SENSOR_EN, "A=%d ", ack);

    if (!ack) {
        sw_i2c_stop();
        APP_DEBUG(DEBUG_SENSOR_EN, "NACK\r\n");
        return 0;
    }

    for (uint8_t i = 0; i < 6; i++) {
        data[i] = sw_i2c_read_byte(i < 5 ? 1 : 0);
    }
    sw_i2c_stop();

    APP_DEBUG(DEBUG_SENSOR_EN, "RAW: %02x %02x %02x %02x %02x %02x\r\n",
        data[0], data[1], data[2], data[3], data[4], data[5]);

    if (crc8(data, 2) != data[2] || crc8(data + 3, 2) != data[5]) {
        APP_DEBUG(DEBUG_SENSOR_EN, "CRC FAIL\r\n");
        return 0;
    }

    *temp_raw = ((uint16_t)data[0] << 8) | data[1];
    *hum_raw = ((uint16_t)data[3] << 8) | data[4];
    return 1;
}

/*********************************************************************
 * @fn      sht30_read
 *
 * @brief   Read temperature and humidity from SHT30 sensor and convert
 *          to ZCL-formatted values (temperature in 0.01C, humidity in 0.01%).
 *
 * @param   temp_zcl - pointer to store temperature in ZCL format
 * @param   hum_zcl  - pointer to store humidity in ZCL format
 *
 * @return  1 on success, 0 on failure
 */
uint8_t sht30_read(int16_t *temp_zcl, uint16_t *hum_zcl) {
    uint16_t temp_raw, hum_raw;

    if (!sht30_measure_and_read(&temp_raw, &hum_raw))
        return 0;

    *temp_zcl = (int16_t)((int32_t)temp_raw * 17500 / 65535 - 4500);
    *hum_zcl = (uint16_t)((int32_t)hum_raw * 10000 / 65535);
    return 1;
}

