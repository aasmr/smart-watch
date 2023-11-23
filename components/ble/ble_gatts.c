/*
 * ble_gatts.c
 *
 *  Created on: 18 нояб. 2023 г.
 *      Author: aasmr
 */

#include "ble_gatts.h"

static const char *TAG = "ble_gatts";

uint8_t tyme_sync_service_uuid[ESP_UUID_LEN_128] = {0x95, 0xb8, 0x4b, 0xe1, 0xe4, 0x34, 0x35, 0x97, 0x4c, 0x40, 0x12, 0x91, 0xff, 0x4d, 0xa8, 0xfa};
uint8_t tyme_sync_characteristic_uuid[ESP_UUID_LEN_128] = {0x95, 0xb8, 0x4b, 0xe1, 0xe4, 0x34, 0x35, 0x97, 0x4c, 0x40, 0x12, 0x91, 0xff, 0x4d, 0xa8, 0xfb};

static void gatts_profile_rgb_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
};

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_TSYNC_APP_ID] = {
        .gatts_cb = gatts_profile_rgb_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    }
};

static void create_time_sync_service(esp_gatt_if_t gatts_if)
{
    gl_profile_tab[PROFILE_TSYNC_APP_ID].service_id.is_primary = true;
    gl_profile_tab[PROFILE_TSYNC_APP_ID].service_id.id.inst_id = 0x00;
    gl_profile_tab[PROFILE_TSYNC_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_128;
    memcpy(gl_profile_tab[PROFILE_TSYNC_APP_ID].service_id.id.uuid.uuid.uuid128, tyme_sync_service_uuid, ESP_UUID_LEN_128);

    esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_TSYNC_APP_ID].service_id, PROFILE_TSYNC_NUM_HANDLE);
}

static void start_time_sync_service(esp_ble_gatts_cb_param_t *param)
{
    gl_profile_tab[PROFILE_TSYNC_APP_ID].service_handle = param->create.service_handle;
    esp_ble_gatts_start_service(gl_profile_tab[PROFILE_TSYNC_APP_ID].service_handle);
}

static void add_characteristic(void)
{
    uint8_t char_value[] = {0x00};

    esp_attr_value_t rgb_char_val =
    {
        .attr_max_len = sizeof(char_value),
        .attr_len     = sizeof(char_value),
        .attr_value   = char_value,
    };

    gl_profile_tab[PROFILE_TSYNC_APP_ID].char_uuid.len = ESP_UUID_LEN_128;
    memcpy(gl_profile_tab[PROFILE_TSYNC_APP_ID].char_uuid.uuid.uuid128, tyme_sync_characteristic_uuid, ESP_UUID_LEN_128);

    esp_err_t add_char_ret = esp_ble_gatts_add_char(gl_profile_tab[PROFILE_TSYNC_APP_ID].service_handle, &gl_profile_tab[PROFILE_TSYNC_APP_ID].char_uuid,
                                                    ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                                    ESP_GATT_CHAR_PROP_BIT_WRITE,
                                                    &rgb_char_val, NULL);
    if (add_char_ret){
        ESP_LOGE(TAG, "add char failed, error code =%x",add_char_ret);
    }
}

static void handle_write_event(uint8_t * p_data, uint16_t len)
{
    uint8_t h = p_data[0];
    uint8_t m = p_data[1];
    uint8_t s = p_data[2];
    //ESP_LOGI(TAG, "opcode = 0x%x", opcode);

    watch_sync(h, m, s);
}

static void gatts_profile_rgb_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(TAG, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
        ble_adv_config();
        create_time_sync_service(gatts_if);
        break;
    case ESP_GATTS_READ_EVT: {
        ESP_LOGI(TAG, "GATT_READ_EVT, conn_id %u, trans_id %u, handle %u\n", (unsigned int)param->read.conn_id, (unsigned int)param->read.trans_id, (unsigned int)param->read.handle);
        break;
    }
    case ESP_GATTS_WRITE_EVT: {
        ESP_LOGI(TAG, "GATT_WRITE_EVT, conn_id %u, trans_id %u, handle %u", (unsigned int)param->write.conn_id, (unsigned int)param->write.trans_id, (unsigned int)param->write.handle);
        if (!param->write.is_prep){
            ESP_LOGI(TAG, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
            esp_log_buffer_hex(TAG, param->write.value, param->write.len);
            handle_write_event(param->write.value, param->write.len);
        }
        esp_gatt_status_t status = ESP_GATT_OK;
        if (param->write.need_rsp){
            if (!param->write.is_prep){
                esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, NULL);
            }
        }
        break;
    }
    case ESP_GATTS_EXEC_WRITE_EVT:
        ESP_LOGI(TAG,"ESP_GATTS_EXEC_WRITE_EVT");
        break;
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
        break;
    case ESP_GATTS_UNREG_EVT:
        break;
    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n", param->create.status, param->create.service_handle);
        start_time_sync_service(param);
        add_characteristic();
        break;
    case ESP_GATTS_ADD_INCL_SRVC_EVT:
        break;
    case ESP_GATTS_ADD_CHAR_EVT: {
        ESP_LOGI(TAG, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d\n",
                param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
        break;
    }
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        ESP_LOGI(TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
                 param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
        break;
    case ESP_GATTS_DELETE_EVT:
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(TAG, "SERVICE_START_EVT, status %d, service_handle %d\n",
                 param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_STOP_EVT:
        break;
    case ESP_GATTS_CONNECT_EVT: {
        esp_ble_conn_update_params_t conn_params = {0};
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
        conn_params.latency = 0;
        conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
        conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
        conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
        ESP_LOGI(TAG, "ESP_GATTS_CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x:",
                 param->connect.conn_id,
                 param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                 param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
        gl_profile_tab[PROFILE_TSYNC_APP_ID].conn_id = param->connect.conn_id;
        //start sent the update connection parameters to the peer device.
        esp_ble_gap_update_conn_params(&conn_params);
        break;
    }
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_DISCONNECT_EVT, disconnect reason 0x%x", param->disconnect.reason);
        ble_adv_start();
        break;
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_CONF_EVT, status %d attr_handle %d", param->conf.status, param->conf.handle);
        if (param->conf.status != ESP_GATT_OK){
            esp_log_buffer_hex(TAG, param->conf.value, param->conf.len);
        }
        break;
    case ESP_GATTS_OPEN_EVT:
    case ESP_GATTS_CANCEL_OPEN_EVT:
    case ESP_GATTS_CLOSE_EVT:
    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
    default:
        break;
    }
}


static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gl_profile_tab[param->reg.app_id].gatts_if = gatts_if;
        } else {
            ESP_LOGI(TAG, "Reg app failed, app_id %04x, status %d\n",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }

    /* If the gatts_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            if (gatts_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                    gatts_if == gl_profile_tab[idx].gatts_if) {
                if (gl_profile_tab[idx].gatts_cb) {
                    gl_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}

void ble_gatts_register(void)
{
    esp_err_t ret;

    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret){
        ESP_LOGE(TAG, "gatts register error, error code = %x", ret);
        return;
    }
}

void ble_gatts_apps_register(void)
{
    esp_err_t ret;

    ret = esp_ble_gatts_app_register(PROFILE_TSYNC_APP_ID);
    if (ret){
        ESP_LOGE(TAG, "gatts app register error, error code = %x", ret);
        return;
    }
}

void ble_gatts_set_mtu(void)
{
    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret){
        ESP_LOGE(TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
    }
}


