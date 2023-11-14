/*
 * spiffs.h
 *
 *  Created on: 14 нояб. 2023 г.
 *      Author: aasmr
 */

#ifndef COMPONENTS_MCU_PHERIPHERAL_SPIFFS_H_
#define COMPONENTS_MCU_PHERIPHERAL_SPIFFS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "esp_spiffs.h"
#include "spiffs_config.h"


void spiffs_init(void);

#endif /* COMPONENTS_MCU_PHERIPHERAL_SPIFFS_H_ */
