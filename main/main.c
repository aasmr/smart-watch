#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <freertos/task.h>
#include <dirent.h>

#include "driver/gpio.h"
#include "spi.h"
#include "tim.h"
#include "lcd.h"
#include "spiffs.h"
#include "watch.h"



void app_main(void)
{
	//Init pereferial
	spi_device_handle_t spi = NULL;
	gptimer_handle_t tim = NULL;
	queue_t ele = {
			.flag = 0,
	};
	QueueHandle_t queue = xQueueCreate(10, sizeof(queue_t));

	spi_config(&spi);
	tim_init(&tim, &queue);

	//Init board
	lcd_init(&spi);
	spiffs_init();

	//Init app
	watch_app_init(&spi);

	//Start pherepherias, board and app
	tim_start(&tim);

	while(1)
	{
    	if (xQueueReceive(queue, &ele, pdMS_TO_TICKS(1)))
    	{
    		if(ele.flag == 1)
    		{
    			watch_app_worker(&spi);
    		}
    	}

	}

}
