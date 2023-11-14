/*
 * lcd.h
 *
 *  Created on: 8 нояб. 2023 г.
 *      Author: aasmr
 */

#ifndef COMPONENTS_LCD_LCD_H_
#define COMPONENTS_LCD_LCD_H_

#include "lcd_Defines.h"

void lcd_reset();
void lcd_cmd(spi_device_handle_t spi, const uint8_t cmd, bool keep_cs_active);
void lcd_data(spi_device_handle_t spi, const uint8_t *data, int len);
void lcd_init_seq(spi_device_handle_t* spi, lcd_init_cmd_t *cmd_init);
void lcd_draw_all(spi_device_handle_t* spi, uint16_t* image);
void lcd_clear(spi_device_handle_t* spi, uint16_t color);
void lcd_set_bgnd(spi_device_handle_t* spi);
void lcd_init();

#endif /* COMPONENTS_LCD_LCD_H_ */
