#include "app_main.h"

#if (CLOCK_SYS_CLOCK_HZ > 24000000)
#define I2C_TCLK_US     10
#else
#define I2C_TCLK_US     32
#endif

/*********************************************************************
 * @fn      i2c_delay
 *
 * @brief   Software delay for I2C timing using hardware tick register.
 *
 * @param   us16 - delay in units of 16 system clock cycles
 *
 * @return  None
 */
_attribute_ram_code_sec_ static void i2c_delay(unsigned int us16) {
    unsigned int t = reg_system_tick;
    while ((unsigned int)(reg_system_tick - t) < us16) {
    }
}

/*********************************************************************
 * @fn      i2c_scl_out
 *
 * @brief   Set the I2C SCL pin as input or output.
 *
 * @param   v - 1 to set as input (release line), 0 to set as output (drive line)
 *
 * @return  None
 */
_attribute_ram_code_sec_ static void i2c_scl_out(int v) {
    gpio_set_output_en(I2C_SCL, v ? 0 : 1);
}

/*********************************************************************
 * @fn      i2c_sda_out
 *
 * @brief   Set the I2C SDA pin as input or output.
 *
 * @param   v - 1 to set as input (release line), 0 to set as output (drive line)
 *
 * @return  None
 */
_attribute_ram_code_sec_ static void i2c_sda_out(int v) {
    gpio_set_output_en(I2C_SDA, v ? 0 : 1);
}

/*********************************************************************
 * @fn      i2c_sda_read
 *
 * @brief   Read the current logic level of the I2C SDA pin.
 *
 * @param   None
 *
 * @return  1 if SDA is high, 0 if SDA is low
 */
_attribute_ram_code_sec_ static int i2c_sda_read(void) {
    return gpio_read(I2C_SDA);
}

/*********************************************************************
 * @fn      sw_i2c_init
 *
 * @brief   Initialize the software I2C interface by configuring SCL and SDA
 *          GPIO pins with pull-up resistors and setting them to idle state.
 *
 * @param   None
 *
 * @return  None
 */
void sw_i2c_init(void) {
    gpio_set_func(I2C_SCL, AS_GPIO);
    gpio_set_output_en(I2C_SCL, 0);
    gpio_set_input_en(I2C_SCL, 1);
    gpio_setup_up_down_resistor(I2C_SCL, PM_PIN_PULLUP_10K);
    gpio_write(I2C_SCL, 0);

    gpio_set_func(I2C_SDA, AS_GPIO);
    gpio_set_output_en(I2C_SDA, 0);
    gpio_set_input_en(I2C_SDA, 1);
    gpio_setup_up_down_resistor(I2C_SDA, PM_PIN_PULLUP_10K);
    gpio_write(I2C_SDA, 0);

    i2c_scl_out(1);
    i2c_sda_out(1);
    i2c_delay(I2C_TCLK_US);
}

/*********************************************************************
 * @fn      soft_i2c_start
 *
 * @brief   Generate an I2C START condition by pulling SDA low while SCL is high.
 *
 * @param   None
 *
 * @return  None
 */
_attribute_ram_code_sec_ static void soft_i2c_start(void) {
    i2c_scl_out(1);
    i2c_sda_out(1);
    i2c_delay(I2C_TCLK_US);
    i2c_sda_out(0);
    i2c_delay(I2C_TCLK_US);
    i2c_scl_out(0);
}

/*********************************************************************
 * @fn      soft_i2c_stop
 *
 * @brief   Generate an I2C STOP condition by releasing SDA while SCL is high.
 *
 * @param   None
 *
 * @return  None
 */
_attribute_ram_code_sec_ static void soft_i2c_stop(void) {
    i2c_sda_out(0);
    i2c_delay(I2C_TCLK_US);
    i2c_scl_out(1);
    i2c_delay(I2C_TCLK_US);
    i2c_sda_out(1);
}

/*********************************************************************
 * @fn      soft_i2c_wr_byte
 *
 * @brief   Write a single byte over the software I2C bus and read ACK.
 *
 * @param   b - byte value to transmit
 *
 * @return  0 if ACK received, 1 if NACK received
 */
_attribute_ram_code_sec_ static int8_t soft_i2c_wr_byte(uint8_t b) {
    int i = 8;
    while (i--) {
        i2c_delay(I2C_TCLK_US / 2);
        i2c_sda_out((b & 0x80) ? 1 : 0);
        i2c_delay(I2C_TCLK_US / 2);
        i2c_scl_out(1);
        i2c_delay(I2C_TCLK_US);
        i2c_scl_out(0);
        b <<= 1;
    }
    i2c_delay(I2C_TCLK_US / 2);
    i2c_sda_out(1);
    i2c_delay(I2C_TCLK_US / 2);
    i2c_scl_out(1);
    i2c_delay(I2C_TCLK_US);
    int ret = i2c_sda_read();
    i2c_scl_out(0);
    return ret ? 1 : 0;
}

/*********************************************************************
 * @fn      soft_i2c_rd_byte
 *
 * @brief   Read a single byte from the software I2C bus and send ACK or NACK.
 *
 * @param   ack - 1 to send ACK, 0 to send NACK
 *
 * @return  Byte read from the bus
 */
_attribute_ram_code_sec_ static uint8_t soft_i2c_rd_byte(int ack) {
    int i = 8;
    uint8_t ret = 0;
    i2c_sda_out(1);
    while (i--) {
        i2c_delay(I2C_TCLK_US);
        i2c_scl_out(1);
        i2c_delay(I2C_TCLK_US);
        ret <<= 1;
        if (i2c_sda_read()) {
            ret |= 1;
        }
        i2c_scl_out(0);
    }
    i2c_delay(I2C_TCLK_US / 2);
    i2c_sda_out(ack ? 0 : 1);
    i2c_delay(I2C_TCLK_US / 2);
    i2c_scl_out(1);
    i2c_delay(I2C_TCLK_US);
    i2c_scl_out(0);
    i2c_sda_out(1);
    return ret;
}

/*********************************************************************
 * @fn      sw_i2c_start
 *
 * @brief   Generate an I2C START condition on the software I2C bus.
 *
 * @param   None
 *
 * @return  None
 */
void sw_i2c_start(void) {
    soft_i2c_start();
}

/*********************************************************************
 * @fn      sw_i2c_stop
 *
 * @brief   Generate an I2C STOP condition on the software I2C bus.
 *
 * @param   None
 *
 * @return  None
 */
void sw_i2c_stop(void) {
    soft_i2c_stop();
}

/*********************************************************************
 * @fn      sw_i2c_write_byte
 *
 * @brief   Write a single byte over the software I2C bus.
 *
 * @param   byte - byte value to transmit
 *
 * @return  1 on ACK, 0 on NACK
 */
uint8_t sw_i2c_write_byte(uint8_t byte) {
    return soft_i2c_wr_byte(byte) ? 0 : 1;
}

/*********************************************************************
 * @fn      sw_i2c_read_byte
 *
 * @brief   Read a single byte from the software I2C bus.
 *
 * @param   ack - 1 to send ACK, 0 to send NACK
 *
 * @return  Byte read from the bus
 */
uint8_t sw_i2c_read_byte(uint8_t ack) {
    return soft_i2c_rd_byte(ack);
}

/*********************************************************************
 * @fn      i2c_write_buf
 *
 * @brief   Write a buffer of data to an I2C device with a START and STOP.
 *
 * @param   addr_w - I2C device write address
 * @param   data   - pointer to data buffer to transmit
 * @param   len    - number of bytes to transmit
 *
 * @return  None
 */
void i2c_write_buf(uint8_t addr_w, uint8_t *data, uint8_t len) {
    sw_i2c_start();
    sw_i2c_write_byte(addr_w);
    for (uint8_t i = 0; i < len; i++) {
        sw_i2c_write_byte(data[i]);
    }
    sw_i2c_stop();
}
