#ifndef SRC_INCLUDE_APP_LCD_H_
#define SRC_INCLUDE_APP_LCD_H_

void lcd_clear(void);
void lcd_show_all(void);
void lcd_write_data(uint8_t *data, uint8_t len);
void lcd_update_bytes(uint8_t offset, uint8_t *data, uint8_t count);
void lcd_battery_set(uint8_t on);
void lcd_link_icon_set(uint8_t on);
void lcd_show_single_bit(uint8_t bit_index);
void lcd_update_temp_hum(int temp_x10, uint8_t hum);

void light_blink_start(uint8_t times, uint16_t ledOnTime, uint16_t ledOffTime);
void light_blink_stop(void);

#if DEBUG_TEST_TEMP_EN
void test_temp_lcd(void);
#endif

#endif /* SRC_INCLUDE_APP_LCD_H_ */
