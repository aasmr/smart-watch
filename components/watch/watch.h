/*
 * watch.h
 *
 *  Created on: 9 нояб. 2023 г.
 *      Author: aasmr
 */

#ifndef COMPONENTS_WATCH_WATCH_H_
#define COMPONENTS_WATCH_WATCH_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <freertos/task.h>
#include <dirent.h>
#include "spi.h"
#include "lcd.h"
#include "esp_log.h"

typedef enum{
	BCKGNG 		= 0,
	SEC	   		= 1,
	MIN	   		= 2,
	HRS    		= 3,
	PART_BCKGND = 4,
} img_type_t;

typedef struct{
	uint8_t		width;
	uint8_t		height;
	uint8_t		xc;
	uint8_t		yc;
	img_type_t	type;
	uint16_t 	*img_arr;
}img16_t;

typedef struct{
	uint8_t		width;
	uint8_t		height;
	uint8_t		xc;
	uint8_t		yc;
	img_type_t	type;
	uint32_t 	*img_arr;
}img32_t;

typedef struct{
	uint8_t		hrs;
	uint8_t		min;
	uint8_t		sec;

	img16_t		cyfer_img;
	img32_t		sec_img;
	img32_t		min_img;
	img32_t		hrs_img;

	img16_t		buf_bgnd;
	img16_t		prev_bgnd_hrs;
	img16_t		next_bgnd_hrs;
	img16_t		next_part_hrs;
	img16_t		prev_bgnd_min;
	img16_t		next_bgnd_min;
	img16_t		next_part_min;
	img32_t		prev_min;
	img16_t		prev_bgnd_sec;
	img16_t		next_bgnd_sec;
	img16_t		next_part_sec;

}watch_t;

void watch_app_init(spi_device_handle_t* spi);
void watch_app_worker(spi_device_handle_t* spi);

#endif /* COMPONENTS_WATCH_WATCH_H_ */
