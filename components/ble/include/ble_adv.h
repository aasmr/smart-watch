/*
 * ble_adv.h
 *
 *  Created on: 18 нояб. 2023 г.
 *      Author: aasmr
 */

#ifndef COMPONENTS_BLE_INCLUDE_BLE_ADV_H_
#define COMPONENTS_BLE_INCLUDE_BLE_ADV_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_gap_ble_api.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"


#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

void ble_adv_config(void);
void ble_adv_start(void);
void ble_adv_set_complete_handle(void);


#endif /* COMPONENTS_BLE_INCLUDE_BLE_ADV_H_ */
