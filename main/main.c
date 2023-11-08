#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <freertos/task.h>
#include "../components/mcu_pheripheral/spi.h"
//#include "esp_wifi.h"
//#include "esp_system.h"
//#include "esp_event.h"
//#include "esp_event_loop.h"
//#include "nvs_flash.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "lcd.h"

typedef struct {
	uint8_t flag;
} queue_t;


static bool IRAM_ATTR tim_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
	BaseType_t high_task_awoken = pdFALSE;
	QueueHandle_t queue = (QueueHandle_t)user_data;

    queue_t ele = {
        .flag = 1,
    };
	xQueueSendFromISR(queue, &ele, &high_task_awoken);

	return high_task_awoken == pdTRUE;
}

void tim_init(gptimer_handle_t* tim)
{

	gptimer_config_t timer_config = {
	    .clk_src = GPTIMER_CLK_SRC_DEFAULT,
	    .direction = GPTIMER_COUNT_UP,
	    .resolution_hz = 1 * 1000*1000, // 1MHz, 1 tick = 1us
	};

	gptimer_alarm_config_t alarm_config = {
	    .reload_count = 0, // counter will reload with 0 on alarm event
	    .alarm_count = 5000000, // period = 1s @resolution 1MHz
	    .flags.auto_reload_on_alarm = true, // enable auto-reload
	};

	ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, tim));
	ESP_ERROR_CHECK(gptimer_set_alarm_action(*tim, &alarm_config));
}

void app_main(void)
{
	//Init pereferial
	spi_device_handle_t spi;
	spi_config(&spi);

	//OLD

	gptimer_handle_t tim = NULL;

	queue_t ele = {
			.flag = 0,
	};
	QueueHandle_t queue = xQueueCreate(10, sizeof(queue_t));

	tim_init(&tim);
	gptimer_event_callbacks_t cbs = {
	    .on_alarm = tim_callback, // register user callback
	};

	ESP_ERROR_CHECK(gptimer_register_event_callbacks(tim, &cbs, queue));
	ESP_ERROR_CHECK(gptimer_enable(tim));
	ESP_ERROR_CHECK(gptimer_start(tim));

	lcd_init(&spi);
}
