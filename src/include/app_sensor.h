#ifndef SRC_INCLUDE_APP_SENSOR_H_
#define SRC_INCLUDE_APP_SENSOR_H_

void app_sensor_measurement(void);
void app_sensor_set_temperature(void);
void app_sensor_set_humidity(void);
uint16_t app_sensor_get_period(void);
int16_t app_sensor_get_temperature(void);
int16_t app_sensor_get_humidity(void);

#endif /* SRC_INCLUDE_APP_SENSOR_H_ */
