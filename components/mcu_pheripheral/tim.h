/*
 * tim.h
 *
 *  Created on: 15 нояб. 2023 г.
 *      Author: aasmr
 */

#ifndef COMPONENTS_MCU_PHERIPHERAL_TIM_H_
#define COMPONENTS_MCU_PHERIPHERAL_TIM_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <freertos/task.h>
#include "driver/gptimer.h"

typedef struct {
	uint8_t flag;
} queue_t;

void tim_init(gptimer_handle_t* tim, QueueHandle_t* queue);
void tim_start(gptimer_handle_t* tim);
void tim_stop(gptimer_handle_t* tim);

#endif /* COMPONENTS_MCU_PHERIPHERAL_TIM_H_ */
