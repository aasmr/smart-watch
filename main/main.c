#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <freertos/task.h>
#include "../components/mcu_pheripheral/spi.h"
#include <dirent.h>
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "lcd.h"
#include "spiffs.h"

static const char *TAG = "main";

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

uint16_t* merging_layers(uint16_t* img1, uint32_t* img2, uint8_t w, uint8_t h)
{
	uint16_t *sum_img;

	sum_img = (uint16_t*) heap_caps_malloc(w*h*sizeof(uint16_t), MALLOC_CAP_SPIRAM);

	int y, x;
	float a, r, g, b;
	uint16_t old_color;
	uint16_t new_color;

	for(y = 0; y < h; y++)
	{
		for(x = 0; x < w; x++)
		{
			a = (float)((img2[w*y+x] >> 8) & 0xFF);
			old_color = (img2[w*y+x] >> 16) & 0xFF;
			old_color = ((old_color >> 8) & 0xFF) + ((old_color & 0xFF) << 8);
			b = (float)((old_color >> 11) & 0x1F);
			g = (float)((old_color >> 5) & 0x3F);
			r = (float)((old_color >> 0) & 0x1F);

			if( a!= 0 )
			{
				new_color = (((uint16_t)(r*(a/255))&0x1F) + (((uint16_t)(g*(a/255))&0x3F) << 5) + (((uint16_t)(b*(a/255))&0x1F) << 11));
				new_color = ((new_color >> 8) & 0xFF) + ((new_color & 0xFF) << 8);
				sum_img[w*y+x] = new_color;
			}
			else
			{
				sum_img[w*y+x] = img1[w*y+x];
			}
		}
	}

	return sum_img;
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
	//ESP_ERROR_CHECK(gptimer_enable(tim));
	//ESP_ERROR_CHECK(gptimer_start(tim));

	lcd_init(&spi);
	spiffs_init();

	FILE* f;
	ESP_LOGI(TAG, "Opening file");
	f = fopen("/spiffs/cyfer", "rb");
	if (f == NULL)
	{
		ESP_LOGE(TAG, "Failed to open file for reading");
	    return;
	}

	uint8_t header[5];
	fread(header, sizeof(header), 1, f);
	uint16_t *cyfer;
	cyfer = (uint16_t*) heap_caps_malloc(240*240*sizeof(uint16_t), MALLOC_CAP_SPIRAM);
	assert(cyfer != NULL);
	ESP_LOGI(TAG, "malloc ok");
	fread(cyfer, 2*240*240, 1, f);
	fclose(f);
	memset(header, 0, sizeof(header));
	lcd_draw_all(&spi, cyfer);

	ESP_LOGI(TAG, "Opening file");
	f = fopen("/spiffs/sec", "rb");
	if (f == NULL)
	{
		ESP_LOGE(TAG, "Failed to open file for reading");
	    return;
	}

	fread(header,sizeof(header),1,f);
	printf("%hu\n", header[0]);
	printf("%hu\n", header[1]);
	printf("%hu\n", header[2]);
	printf("%hu\n", header[3]);
	printf("%hu\n", header[4]);
	uint32_t *sec;
	sec = (uint32_t*) heap_caps_malloc(header[0]*header[1]*sizeof(uint32_t), MALLOC_CAP_SPIRAM);
	assert(sec != NULL);
	ESP_LOGI(TAG, "malloc ok");
	fread(sec, 4*header[0]*header[1], 1, f);
	fclose(f);

	printf("%u\n", (unsigned int)sec[16]);

	uint16_t *mrg_img;
	uint16_t *part_bg;
	part_bg = (uint16_t*) heap_caps_malloc(header[0]*header[1]*sizeof(uint16_t), MALLOC_CAP_SPIRAM);
	uint8_t x, y, start_x, start_y;
	uint32_t i;
	i = 0;
	start_x = 120-header[3];
	start_y = 120-header[4];
	for (y = start_y; y < start_y + header[1]; y++)
	{
		for (x = start_x; x < start_x + header[0]; x++)
		{
			part_bg[i] = cyfer[240*y+x];
			i++;
		}
	}
	mrg_img = merging_layers(part_bg, sec, header[0], header[1]);

	lcd_draw_all(&spi, cyfer);
	lcd_draw_part_wo_lines(&spi, start_x, start_y, header[0], header[1], mrg_img);
}
