#include "app_main.h"

#define LCD_I2C_ADDR            0x3E
#define LCD_I2C_ADDR_W          ((LCD_I2C_ADDR << 1) | 0)
#define LCD_I2C_ADDR_R          ((LCD_I2C_ADDR << 1) | 1)

#define LCD_CMD_LEN             6
#define LCD_DATA_LEN            5

#define SEG_COUNT               48

static uint8_t lcd_buf[4 + 16];
static uint8_t lcd_frame[6];

void lcd_link_icon_set(uint8_t on);

/*********************************************************************
 * @fn      lcd_send
 *
 * @brief   Send data buffer to LCD via I2C
 *
 * @param   buf - pointer to data buffer to send
 * @param   len - length of data to send
 *
 * @return  None
 */
static void lcd_send(uint8_t *buf, uint8_t len) {
    i2c_write_buf(LCD_I2C_ADDR_W, buf, len);
}

/*********************************************************************
 * @fn      lcd_flush
 *
 * @brief   Flush LCD frame buffer to the display
 *
 * @param   None
 *
 * @return  None
 */
static void lcd_flush(void) {
    lcd_buf[0] = 0xB6;
    lcd_buf[1] = 0xFC;
    lcd_buf[2] = 0xC8;
    lcd_buf[3] = 0xE8;
    lcd_buf[4] = 0x08;
    for (uint8_t i = 0; i < 6; i++) {
        lcd_buf[5 + i] = lcd_frame[i];
    }
    APP_DEBUG(DEBUG_LCD_EN, "LCD RAW: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\r\n",
        lcd_buf[0], lcd_buf[1], lcd_buf[2], lcd_buf[3], lcd_buf[4],
        lcd_buf[5], lcd_buf[6], lcd_buf[7], lcd_buf[8], lcd_buf[9], lcd_buf[10]);
    lcd_send(lcd_buf, 5 + 6);
}

static const uint8_t seg7[] = {
    0x3F, 0x06, 0x5B, 0x4F,
    0x66, 0x6D, 0x7D, 0x07,
    0x7F, 0x6F
};

/*********************************************************************
 * @fn      digit_to_seg1
 *
 * @brief   Convert a digit to segment pattern for the first digit position
 *
 * @param   digit - digit value (0-9)
 * @param   dot - flag to enable decimal point (1=on, 0=off)
 *
 * @return  segment pattern byte for the digit
 */
static uint8_t digit_to_seg1(uint8_t digit, uint8_t dot) {
    uint8_t s = seg7[digit];
    uint8_t b = 0;
    if (s & 0x01) b |= (1 << 4);
    if (s & 0x02) b |= (1 << 0);
    if (s & 0x04) b |= (1 << 2);
    if (s & 0x08) b |= (1 << 7);
    if (s & 0x10) b |= (1 << 6);
    if (s & 0x20) b |= (1 << 5);
    if (s & 0x40) b |= (1 << 1);
    if (dot) b |= (1 << 3);
    return b;
}

/*********************************************************************
 * @fn      lcd_link_blink_cb
 *
 * @brief   Callback function for link icon blinking timer
 *
 * @param   arg - user-defined argument (unused)
 *
 * @return  next interval in ms, or -1 to stop timer
 */
static int32_t lcd_link_blink_cb(void *arg) {
    if (g_appCtx.times == 0) {
        g_appCtx.timerLedEvt = NULL;
        return -1;
    }

    if (g_appCtx.sta == g_appCtx.oriSta) {
        g_appCtx.times--;
        if (g_appCtx.times <= 0) {
            lcd_link_icon_set(g_appCtx.oriSta);
            g_appCtx.timerLedEvt = NULL;
            return -1;
        }
    }

    g_appCtx.sta = !g_appCtx.sta;
    lcd_link_icon_set(g_appCtx.sta);

    if (g_appCtx.sta) return g_appCtx.ledOnTime;
    else              return g_appCtx.ledOffTime;
}

/*********************************************************************
 * @fn      lcd_show_all
 *
 * @brief   Turn on all LCD segments
 *
 * @param   None
 *
 * @return  None
 */
void lcd_show_all(void) {
    lcd_buf[0] = 0xC8;
    lcd_buf[1] = 0xEC;
    lcd_buf[2] = 0xE8;
    lcd_buf[3] = 0x08;
    for (uint8_t i = 0; i < 16; i++) {
        lcd_buf[4 + i] = 0xFF;
    }
    APP_DEBUG(DEBUG_LCD_EN, "LCD ALL: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\r\n",
        lcd_buf[0], lcd_buf[1], lcd_buf[2], lcd_buf[3], lcd_buf[4],
        lcd_buf[5], lcd_buf[6], lcd_buf[7], lcd_buf[8], lcd_buf[9],
        lcd_buf[10], lcd_buf[11], lcd_buf[12], lcd_buf[13], lcd_buf[14],
        lcd_buf[15], lcd_buf[16], lcd_buf[17], lcd_buf[18], lcd_buf[19]);
    lcd_send(lcd_buf, 4 + 16);
}

/*********************************************************************
 * @fn      lcd_clear
 *
 * @brief   Clear all LCD segments
 *
 * @param   None
 *
 * @return  None
 */
void lcd_clear(void) {
    lcd_buf[0] = 0xC8;
    lcd_buf[1] = 0xEC;
    lcd_buf[2] = 0xE8;
    lcd_buf[3] = 0x08;
    for (uint8_t i = 0; i < 16; i++) {
        lcd_buf[4 + i] = 0x00;
    }
    APP_DEBUG(DEBUG_LCD_EN, "LCD CLR: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\r\n",
        lcd_buf[0], lcd_buf[1], lcd_buf[2], lcd_buf[3], lcd_buf[4],
        lcd_buf[5], lcd_buf[6], lcd_buf[7], lcd_buf[8], lcd_buf[9],
        lcd_buf[10], lcd_buf[11], lcd_buf[12], lcd_buf[13], lcd_buf[14],
        lcd_buf[15], lcd_buf[16], lcd_buf[17], lcd_buf[18], lcd_buf[19]);
    lcd_send(lcd_buf, 4 + 16);
    for (uint8_t i = 0; i < 6; i++) lcd_frame[i] = 0;
}

/*********************************************************************
 * @fn      lcd_write_data
 *
 * @brief   Write data to LCD frame buffer and flush to display
 *
 * @param   data - pointer to data buffer
 * @param   len - length of data (max 6)
 *
 * @return  None
 */
void lcd_write_data(uint8_t *data, uint8_t len) {
    for (uint8_t i = 0; i < 6 && i < len; i++) {
        lcd_frame[i] = data[i];
    }
    APP_DEBUG(DEBUG_LCD_EN, "WR: %02x %02x %02x %02x %02x %02x\r\n",
        lcd_frame[0], lcd_frame[1], lcd_frame[2],
        lcd_frame[3], lcd_frame[4], lcd_frame[5]);
    lcd_flush();
}

/*********************************************************************
 * @fn      lcd_update_bytes
 *
 * @brief   Update specific bytes in LCD frame buffer and flush
 *
 * @param   offset - byte offset in frame buffer (0-5)
 * @param   data - pointer to data to write
 * @param   count - number of bytes to update
 *
 * @return  None
 */
void lcd_update_bytes(uint8_t offset, uint8_t *data, uint8_t count) {
    for (uint8_t i = 0; i < count && (offset + i) < 6; i++) {
        lcd_frame[offset + i] = data[i];
    }
    lcd_flush();
}

/*********************************************************************
 * @fn      lcd_battery_set
 *
 * @brief   Set or clear battery icon on LCD
 *
 * @param   on - 1 to show battery icon, 0 to hide
 *
 * @return  None
 */
void lcd_battery_set(uint8_t on) {
    uint8_t val = lcd_frame[3];
    if (on == ((val >> 5) & 1)) return;
    if (on) val |= 0x20;
    else    val &= ~0x20;
    lcd_update_bytes(3, &val, 1);
}

/*********************************************************************
 * @fn      lcd_show_single_bit
 *
 * @brief   Display a single segment by bit index
 *
 * @param   bit_index - segment bit index (0-47)
 *
 * @return  None
 */
void lcd_show_single_bit(uint8_t bit_index) {
    uint8_t data[6] = {0, 0, 0, 0, 0, 0};
    uint8_t byte_idx = bit_index / 8;
    uint8_t bit_pos = bit_index % 8;
    if (byte_idx < 6) {
        data[byte_idx] = (1 << bit_pos);
    }
    lcd_write_data(data, 6);
}

/*********************************************************************
 * @fn      lcd_update_temp_hum
 *
 * @brief   Update temperature and humidity display on LCD
 *
 * @param   temp_x10 - temperature value multiplied by 10 (e.g., 255 = 25.5C)
 * @param   hum - humidity percentage (0-99)
 *
 * @return  None
 */
void lcd_update_temp_hum(int temp_x10, uint8_t hum) {
    uint16_t abs_x10, d0, d2, d3;
    uint8_t buf[6];

    if (hum > 99) hum = 99;

    if (temp_x10 < -99 || temp_x10 > 1999) {
        buf[0] = 0xF2;
        buf[1] = 0x42;
        buf[2] = 0;
        buf[3] = (lcd_frame[3] & 0x20) | 0x40;
        buf[4] = digit_to_seg1(hum / 10, 0) | (lcd_frame[4] & 0x08);
        buf[5] = digit_to_seg1(hum % 10, 1);
        lcd_write_data(buf, 6);
        return;
    }

    if (temp_x10 < 0) {
        abs_x10 = (uint16_t)(-temp_x10);
        d0 = (abs_x10 / 100) % 10;
        buf[0] = d0 ? digit_to_seg1(d0, 0) : 0;
        buf[0] |= (1 << 1);
    } else if (temp_x10 >= 1000) {
        abs_x10 = (uint16_t)temp_x10;
        d0 = abs_x10 / 1000;
        buf[0] = digit_to_seg1((abs_x10 / 100) % 10, 0);
        if (d0) buf[0] |= (1 << 3);
    } else {
        abs_x10 = (uint16_t)temp_x10;
        d0 = (abs_x10 / 100) % 10;
        buf[0] = d0 ? digit_to_seg1(d0, 0) : 0;
    }

    d2 = (abs_x10 / 10) % 10;
    d3 = abs_x10 % 10;

    buf[1] = digit_to_seg1(d2, d3 ? 1 : 0);
    buf[2] = d3 ? digit_to_seg1(d3, 1) : 0;
    buf[3] = (lcd_frame[3] & 0x20) | 0x40;
    buf[4] = digit_to_seg1(hum / 10, 0) | (lcd_frame[4] & 0x08);
    buf[5] = digit_to_seg1(hum % 10, 1);

    lcd_write_data(buf, 6);
}

/*********************************************************************
 * @fn      lcd_link_icon_set
 *
 * @brief   Set or clear link icon on LCD
 *
 * @param   on - 1 to show link icon, 0 to hide
 *
 * @return  None
 */
void lcd_link_icon_set(uint8_t on) {
    uint8_t val = lcd_frame[4];
    if (on == ((val >> 3) & 1)) return;
    if (on) val |= 0x08;
    else    val &= ~0x08;
    lcd_update_bytes(4, &val, 1);
}

/*********************************************************************
 * @fn      light_blink_start
 *
 * @brief   Start blinking the link icon LED
 *
 * @param   times - number of blink cycles
 * @param   ledOnTime - LED on duration in ms
 * @param   ledOffTime - LED off duration in ms
 *
 * @return  None
 */
void light_blink_start(uint8_t times, uint16_t ledOnTime, uint16_t ledOffTime) {
    uint32_t interval = 0;
    g_appCtx.times = times;

    if (!g_appCtx.timerLedEvt) {
        if (g_appCtx.oriSta) {
            lcd_link_icon_set(OFF);
            g_appCtx.sta = 0;
            interval = ledOffTime;
        } else {
            lcd_link_icon_set(ON);
            g_appCtx.sta = 1;
            interval = ledOnTime;
        }
        g_appCtx.ledOnTime = ledOnTime;
        g_appCtx.ledOffTime = ledOffTime;
        g_appCtx.timerLedEvt = TL_ZB_TIMER_SCHEDULE(lcd_link_blink_cb, NULL, interval);
    }
}

/*********************************************************************
 * @fn      light_blink_stop
 *
 * @brief   Stop the LED blink timer and restore original state
 *
 * @param   None
 *
 * @return  None
 */
void light_blink_stop(void) {
    if (g_appCtx.timerLedEvt) {
        TL_ZB_TIMER_CANCEL(&g_appCtx.timerLedEvt);
        g_appCtx.timerLedEvt = NULL;
        g_appCtx.times = 0;
        lcd_link_icon_set(g_appCtx.oriSta);
    }
}

#if UART_PRINTF_MODE && DEBUG_TEST_TEMP_EN
/*********************************************************************
 * @fn      test_lcd
 *
 * @brief   Test LCD temperature display with various values
 *
 * @param   None
 *
 * @return  None
 */
void test_temp_lcd(void) {
    lcd_update_temp_hum(-400, 50);
    sleep_ms(2000);
    lcd_show_all();
    sleep_ms(1000);
    lcd_update_temp_hum(-215, 50);
    sleep_ms(2000);
    lcd_show_all();
    sleep_ms(1000);
    lcd_update_temp_hum(-100, 50);
    sleep_ms(2000);
    lcd_show_all();
    sleep_ms(1000);
    lcd_update_temp_hum(-99, 50);
    sleep_ms(2000);
    lcd_show_all();
    sleep_ms(1000);
    lcd_update_temp_hum(-75, 50);
    sleep_ms(2000);
    lcd_show_all();
    sleep_ms(1000);
    lcd_update_temp_hum(-43, 50);
    sleep_ms(2000);
    lcd_show_all();
    sleep_ms(1000);
    lcd_update_temp_hum(0, 50);
    sleep_ms(2000);
    lcd_show_all();
    sleep_ms(1000);
    lcd_update_temp_hum(123, 50);
    sleep_ms(2000);
    lcd_show_all();
    sleep_ms(1000);
    lcd_update_temp_hum(1200, 50);
    sleep_ms(2000);
    lcd_show_all();
    sleep_ms(1000);
}
#endif

