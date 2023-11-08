/*
 * spi_master.h
 *
 *  Created on: 8 нояб. 2023 г.
 *      Author: aasmr
 */

#ifndef COMPONENTS_MCU_PEREFERIAL_SPI_H_
#define COMPONENTS_MCU_PEREFERIAL_SPI_H_

#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"

#define LCD_SPI			    SPI2_HOST
#define LCD_SPI_CS_Pin		10		//GPIO10, Pin15
#define LCD_SPI_MOSI_Pin	11		//GPIO11, Pin16
#define LCD_SPI_MISO_Pin	13		//GPIO13, Pin18
#define LCD_SPI_CLK_Pin		12		//GPIO12, Pin17
#define LCD_DC_Pin			7		//GPIO7, Pin12
#define LCD_RST_Pin			6		//GPIO6, Pin11
#define PARALLEL_LINES 		60

void spi_pre_transfer_callback(spi_transaction_t *trans);
void spi_config(spi_device_handle_t* spi);

#endif /* COMPONENTS_MCU_PEREFERIAL_SPI_H_ */
