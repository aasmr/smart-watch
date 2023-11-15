/*
 * spiffs.c
 *
 *  Created on: 14 нояб. 2023 г.
 *      Author: aasmr
 */

#include "spiffs.h"

static const char *TAG = "spiffs";

void spiffs_init(void)
{
	esp_vfs_spiffs_conf_t conf =
	{
				.base_path = "/spiffs",
				.partition_label = NULL,
				.max_files = 10,
				.format_if_mount_failed = true
	};

	esp_err_t ret = esp_vfs_spiffs_register(&conf);

	if (ret != ESP_OK)
	{
		if (ret == ESP_FAIL)
		{
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		}
		else if (ret == ESP_ERR_NOT_FOUND)
		{
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		}
		else
		{
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
		}

		return;
	}
}

