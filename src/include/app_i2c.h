#ifndef SRC_INCLUDE_APP_I2C_H_
#define SRC_INCLUDE_APP_I2C_H_

void sw_i2c_init(void);
void sw_i2c_start(void);
void sw_i2c_stop(void);
uint8_t sw_i2c_write_byte(uint8_t byte);
uint8_t sw_i2c_read_byte(uint8_t ack);

void i2c_write_buf(uint8_t addr_w, uint8_t *data, uint8_t len);


#endif /* SRC_INCLUDE_APP_I2C_H_ */
