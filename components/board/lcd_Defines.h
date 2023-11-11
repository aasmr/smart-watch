/*
 * lcd_Defines.h
 *
 *  Created on: 8 нояб. 2023 г.
 *      Author: aasmr
 */

#ifndef COMPONENTS_LCD_LCD_DEFINES_H_
#define COMPONENTS_LCD_LCD_DEFINES_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include "spi.h"

#define BLACK					0x0000

#define LCD_HEIGHT 240
#define LCD_WIDTH 240

typedef struct {
    uint8_t cmd_reg;
    uint8_t cmd_data[16];
    uint8_t cmd_len; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} lcd_init_cmd_t;


#endif /* COMPONENTS_LCD_LCD_DEFINES_H_ */
