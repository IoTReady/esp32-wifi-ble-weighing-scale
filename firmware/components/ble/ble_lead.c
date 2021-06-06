//
//  ble_lead.c
//  Weighing_Scale_Firmware
//
//  ESP32 firmware for Smart Weighing Scale
//
//  This module allows the use of BLE to change system settings.
//
//  Created by IoTReady Technology Solutions Pvt. Ltd., Bangalore
//  Copyright © 2019 IoTReady Technology Solutions Pvt. Ltd., Bangalore
//
// All rights reserved. Gaining access to this code without explicit 
// license or through reverse compilation constitutes violation of 
// protected intellectual property rights.
// 
// If you are interested in using this code or similar functionality
// for your product, please contact us at hello@iotready.co
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

// START #INCLUDES
#include <string.h>
#include "esp_log.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "cJSON.h"
#include "ble_common.h"
#include "ble_lead.h"
#include "common.h"
#include "wifi_sta.h"
#include "local_ota.h"
#include "nvs_store.h"
#include "HX711.h"
#include "weighing_scale.h"
#include "battery_level_check.h"

// END #INCLUDES

// START #DEFINES

#define TAG "BLE_LEAD"
#define DEVICE_STATE_SERVICE_UUID            0x00FF
#define WEIGHT_DATA_CHAR_UUID                0xFF00
#define NUM_HANDLE_DEVICE_STATE_SERVICE      4 


#define SYSTEM_SETTINGS_SERVICE_UUID        0x00EE
#define DEVICEID_CHAR_UUID                  0xEE00
#define WIFI_SSID_CHAR_UUID                 0xEE01
#define WIFI_PASSWORD_CHAR_UUID             0xEE02
#define OTA_IP_CHAR_UUID                    0xEE03
#define OTA_PORT_CHAR_UUID                  0xEE04
#define OTA_FILENAME_CHAR_UUID              0xEE05
#define FORCE_OTA_CHAR_UUID                 0xEE06
#define CANCEL_OTA_CHAR_UUID                0xEE07
#define CONNECT_WIFI_CHAR_UUID              0xEE08
#define SOFTWARE_VERSION_CHAR_UUID          0xEE09
#define NUM_HANDLE_SYSTEM_SETTINGS          40


#define TEST_MANUFACTURER_DATA_LEN  17

// DEVICE_STATE_SERVICE
#define WEIGHT_DATA_ID           0

// SYSTEM_SETTINGS_SERVICE
#define DEVICEID_ID             0
#define WIFI_SSID_ID            1
#define WIFI_PASSWORD_ID        2
#define OTA_IP_ID               3
#define OTA_PORT_ID             4
#define OTA_FILENAME_ID         5
#define FORCE_OTA_ID            6
#define CANCEL_OTA_ID           7
#define CONNECT_WIFI_ID         8
#define SOFTWARE_VERSION_ID     9

#define MAX_CHAR_LEN 0x258
#define adv_config_flag      (1 << 0)
#define scan_rsp_config_flag (1 << 1)

// END #DEFINES

// START FUNCTION DECLARATIONS
static void lead_gap_setup_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void lead_gatts_setup_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
static void system_settings_gatts_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
static void device_state_gatts_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
esp_err_t disable_ble_lead();
esp_err_t enable_ble_lead();
esp_err_t set_adv_device_state();
esp_err_t setup_lead_advertising();

// END FUNCTION DECLARATIONS


// START MODULE GLOBAL VARIABLES

esp_gatt_char_prop_t DEVICE_STATE_SERVICE_PROPERTY = 0;
esp_gatt_char_prop_t SYSTEM_SETTINGS_SERVICE_PROPERTY = 0;
esp_gatt_if_t current_gatts_if;
static uint8_t adv_config_done = 0;

static esp_ble_adv_params_t adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    //.peer_addr            =
    //.peer_addr_type       =
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

struct gatts_characteristic_inst{
    esp_bt_uuid_t char_uuid;
    esp_bt_uuid_t descr_uuid;
    uint16_t char_handle;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
};

struct gatts_service_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    struct gatts_characteristic_inst chars[10];
};


/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_service_inst gatts_profile_array[NUM_SERVICES] = {
    [DEVICE_STATE_SERVICE] = {
        .gatts_cb = device_state_gatts_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
    [SYSTEM_SETTINGS_SERVICE] = {
        .gatts_cb = system_settings_gatts_handler,                   /* This demo does not implement, similar as profile A */
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
   
};


// END MODULE GLOBAL VARIABLES

// START FUNCTION DEFINITIONS

static void lead_gap_setup_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    #ifdef CONFIG_SET_RAW_ADV_DATA
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
        adv_config_done &= (~adv_config_flag);
        if (adv_config_done==0){
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
        adv_config_done &= (~scan_rsp_config_flag);
        if (adv_config_done==0){
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    #else
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        adv_config_done &= (~adv_config_flag);
        if (adv_config_done == 0){
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
        adv_config_done &= (~scan_rsp_config_flag);
        if (adv_config_done == 0){
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    #endif
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        //advertising start complete event to indicate advertising start successfully or failed
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "Advertising start failed");
        }
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "Advertising stop failed");
        }
        else {
            ESP_LOGI(TAG, "Stop adv successfully");
        }
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
         ESP_LOGI(TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
        break;
    default:
        break;
    }
}

static void lead_gatts_setup_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gatts_profile_array[param->reg.app_id].gatts_if = gatts_if;
        } else {
            ESP_LOGE(TAG, "Reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }

    /* If the gatts_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    do {
        int idx;
        for (idx = 0; idx < NUM_SERVICES; idx++) {
            if (gatts_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                    gatts_if == gatts_profile_array[idx].gatts_if) {
                if (gatts_profile_array[idx].gatts_cb) {
                    gatts_profile_array[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}

static void system_settings_gatts_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    // esp_err_t err;
    switch (event) {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(TAG, "REGISTER_APP_EVT, status %d, app_id %d", param->reg.status, param->reg.app_id);
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].service_id.is_primary = true;
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].service_id.id.inst_id = 0x00;
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].service_id.id.uuid.len = ESP_UUID_LEN_16;
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].service_id.id.uuid.uuid.uuid16 = SYSTEM_SETTINGS_SERVICE_UUID;
        current_gatts_if = gatts_if;
        // setup_lead_advertising();
        esp_ble_gatts_create_service(gatts_if, &gatts_profile_array[SYSTEM_SETTINGS_SERVICE].service_id, NUM_HANDLE_SYSTEM_SETTINGS);
        break;
    case ESP_GATTS_READ_EVT: {
        ESP_LOGI(TAG, "GATT_READ_EVT, conn_id %d, trans_id %d, handle %d", param->read.conn_id, param->read.trans_id, param->read.handle);
        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
        rsp.attr_value.handle = param->read.handle;
        rsp.attr_value.len = 5;
        uint16_t currentHandle = param->read.handle;
        
        if (currentHandle == gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[DEVICEID_ID].char_handle) 
        {
            // memcpy(rsp.attr_value.value, device_id, 12);
            // ESP_LOGI(TAG, "Sending BLE read response: %s", rsp.attr_value.value);
        }
        else if (currentHandle == gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[WIFI_SSID_ID].char_handle) 
        {
            memcpy(rsp.attr_value.value, wifi_params.network_ssid, sizeof(wifi_params.network_ssid));
            ESP_LOGI(TAG, "Sending BLE read response: %s", rsp.attr_value.value);
        }
        else if (currentHandle == gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[WIFI_PASSWORD_ID].char_handle) 
        {
            memcpy(rsp.attr_value.value, wifi_params.network_password, sizeof(wifi_params.network_password));
            ESP_LOGI(TAG, "Sending BLE read response: %s", rsp.attr_value.value);
        }
        else if (currentHandle == gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[OTA_IP_ID].char_handle) 
        {
            memcpy(rsp.attr_value.value, OTA_IP, sizeof(OTA_IP));
            ESP_LOGI(TAG, "Sending BLE read response: %s", rsp.attr_value.value);
        }
        else if (currentHandle == gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[OTA_PORT_ID].char_handle) 
        {
            memcpy(rsp.attr_value.value, OTA_PORT, sizeof(OTA_PORT));
            ESP_LOGI(TAG, "Sending BLE read response: %s", rsp.attr_value.value);
        }
        else if (currentHandle == gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[OTA_FILENAME_ID].char_handle) 
        {
            memcpy(rsp.attr_value.value, OTA_FILENAME, sizeof(OTA_FILENAME));
            ESP_LOGI(TAG, "Sending BLE read response: %s", rsp.attr_value.value);
        }
        else if (currentHandle == gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[SOFTWARE_VERSION_ID].char_handle) 
        {
            char *software_version_str = malloc(sizeof(char));
            sprintf(software_version_str, "%d", SOFTWARE_VERSION);
            memcpy(rsp.attr_value.value, software_version_str, sizeof(char));
            ESP_LOGI(TAG, "Sending BLE read response: %s", rsp.attr_value.value);
        }
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                    ESP_GATT_OK, &rsp);
        break;
    }
    case ESP_GATTS_WRITE_EVT: {
        ESP_LOGI(TAG, "System Settings: GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d", param->write.conn_id, param->write.trans_id, param->write.handle);
        if (!param->write.is_prep)
        {
            esp_log_buffer_hex(TAG, param->write.value, param->write.len);
            uint16_t writeHandle = param->write.handle;
            // system_settings_t system_settings;
            // err = read_system_settings_nvs(&system_settings);

            if (writeHandle ==  gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[WIFI_SSID_ID].char_handle)
            {
                ESP_LOGI(TAG, "String written is %s", (char *) param->write.value);
                memset(wifi_params.network_ssid, 0, sizeof(wifi_params.network_ssid));
                for (int i=0; i < param->write.len; i++) 
                {
                    wifi_params.network_ssid[i] = param->write.value[i];
                }
            }
            else if (writeHandle ==  gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[WIFI_PASSWORD_ID].char_handle)
            {
                ESP_LOGI(TAG, "String written is %s", (char *) param->write.value);
                memset(wifi_params.network_password, 0, sizeof(wifi_params.network_password));
                for (int i=0; i < param->write.len; i++) 
                {
                    wifi_params.network_password[i] = param->write.value[i];
                }
            }
            else if (writeHandle ==  gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[OTA_IP_ID].char_handle)
            {
                ESP_LOGI(TAG, "String written is %s", (char *) param->write.value);
                memset(OTA_IP, 0, sizeof(OTA_IP));
                for (int i=0; i < param->write.len; i++)
                {
                    OTA_IP[i] = param->write.value[i];
                }
            }
            else if (writeHandle ==  gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[OTA_PORT_ID].char_handle)
            {
                ESP_LOGI(TAG, "String written is %s", (char *) param->write.value);
                memset(OTA_PORT, 0, sizeof(OTA_PORT));
                for (int i=0; i < param->write.len; i++)
                {
                    OTA_PORT[i] = param->write.value[i];
                }
            }
            else if (writeHandle ==  gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[OTA_FILENAME_ID].char_handle)
            {
                ESP_LOGI(TAG, "String written is %s", (char *) param->write.value);
                memset(OTA_FILENAME, 0, sizeof(OTA_FILENAME));
                for (int i=0; i < param->write.len; i++)
                {
                    OTA_FILENAME[i] = param->write.value[i];
                }
            }
            else if (writeHandle == gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[FORCE_OTA_ID].char_handle) 
            {
                ESP_LOGI(TAG, "OTA requested! %s", param->write.value);
                esp_err_t err;
                // WiFi
                err = read_wifi_params_nvs();
                if (err)
                {
                    write_wifi_params_nvs();
                }
                wifi_sta_init(&wifi_params);

                // boot_mode = 1;
                // int count = 0;
                // // display_ota();
                // allow_reboot = true;
                // // give the user 10 seconds to cancel the OTA
                // while (count < 10)
                // {
                //     vTaskDelay(1000/portTICK_PERIOD_MS);
                //     count++;
                // }
                // // 10 seconds have passed and the user hasn't changed anything - via app or GPIO
                // if (allow_reboot == true)
                // {
                //     // write_boot_mode_nvs();
                //     vTaskDelay(1000/portTICK_PERIOD_MS);
                //     esp_restart();
                // }
                xTaskCreate(&local_ota_task, "local_ota_task", LARGE_STACK, NULL, MEDIUM_PRIORITY, local_ota_task_handle);
            }
            else if (writeHandle == gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[CANCEL_OTA_ID].char_handle) 
            {
                // allow_reboot = false;
                display_tare();
                vTaskSuspend(weighingTask_Handle);
                vTaskSuspend(tareTask_Handle);
                vTaskSuspend(battery_level_check_task_handle);                
                vTaskDelay(300/portTICK_PERIOD_MS);
                tare(2);
                write_offset_nvs();
                vTaskDelay(300/portTICK_PERIOD_MS);
                display_clear();                
                vTaskResume(weighingTask_Handle);
                vTaskResume(tareTask_Handle);
                vTaskResume(battery_level_check_task_handle);
            }
            else if (writeHandle == gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[CONNECT_WIFI_ID].char_handle) 
            {
                // esp_err_t err;
                // // WiFi
                // err = read_wifi_params_nvs();
                // if (err)
                // {
                //     write_wifi_params_nvs();
                // }
                wifi_sta_init(&wifi_params);
                // write_wifi_params_nvs();
            }
        }
        else {
            ESP_LOGI(TAG, "Needs write prep?");
        }
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        break;
    }
    case ESP_GATTS_EXEC_WRITE_EVT:
        ESP_LOGI(TAG,"ESP_GATTS_EXEC_WRITE_EVT");
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        break;
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
        break;
    case ESP_GATTS_UNREG_EVT:
        break;
    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d", param->create.status, param->create.service_handle);
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].service_handle = param->create.service_handle;        
        SYSTEM_SETTINGS_SERVICE_PROPERTY = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE ;

    
   //MAC Address CHARACTERISTIC       
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[DEVICEID_ID].char_uuid.len = ESP_UUID_LEN_16;
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[DEVICEID_ID].char_uuid.uuid.uuid16 = DEVICEID_CHAR_UUID;
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[DEVICEID_ID].property = SYSTEM_SETTINGS_SERVICE_PROPERTY;
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[DEVICEID_ID].perm = ESP_GATT_PERM_READ;

    //WiFI SSID CHARACTERISTIC       
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[WIFI_SSID_ID].char_uuid.len = ESP_UUID_LEN_16;
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[WIFI_SSID_ID].char_uuid.uuid.uuid16 = WIFI_SSID_CHAR_UUID;
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[WIFI_SSID_ID].property = SYSTEM_SETTINGS_SERVICE_PROPERTY;
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[WIFI_SSID_ID].perm = ESP_GATT_PERM_READ| ESP_GATT_PERM_WRITE;

    //WiFI Password CHARACTERISTIC       
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[WIFI_PASSWORD_ID].char_uuid.len = ESP_UUID_LEN_16;
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[WIFI_PASSWORD_ID].char_uuid.uuid.uuid16 = WIFI_PASSWORD_CHAR_UUID;
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[WIFI_PASSWORD_ID].property = SYSTEM_SETTINGS_SERVICE_PROPERTY;
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[WIFI_PASSWORD_ID].perm = ESP_GATT_PERM_READ| ESP_GATT_PERM_WRITE;

    //OTA IP CHARACTERISTIC       
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[OTA_IP_ID].char_uuid.len = ESP_UUID_LEN_16;
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[OTA_IP_ID].char_uuid.uuid.uuid16 = OTA_IP_CHAR_UUID;
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[OTA_IP_ID].property = SYSTEM_SETTINGS_SERVICE_PROPERTY;
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[OTA_IP_ID].perm = ESP_GATT_PERM_READ| ESP_GATT_PERM_WRITE;

    //OTA PORT CHARACTERISTIC       
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[OTA_PORT_ID].char_uuid.len = ESP_UUID_LEN_16;
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[OTA_PORT_ID].char_uuid.uuid.uuid16 = OTA_PORT_CHAR_UUID;
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[OTA_PORT_ID].property = SYSTEM_SETTINGS_SERVICE_PROPERTY;
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[OTA_PORT_ID].perm = ESP_GATT_PERM_READ| ESP_GATT_PERM_WRITE;    

     //OTA File Name CHARACTERISTIC       
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[OTA_FILENAME_ID].char_uuid.len = ESP_UUID_LEN_16;
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[OTA_FILENAME_ID].char_uuid.uuid.uuid16 = OTA_FILENAME_CHAR_UUID;
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[OTA_FILENAME_ID].property = SYSTEM_SETTINGS_SERVICE_PROPERTY;
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[OTA_FILENAME_ID].perm = ESP_GATT_PERM_READ| ESP_GATT_PERM_WRITE; 

    // Force OTA CHARACTERISTIC       
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[FORCE_OTA_ID].char_uuid.len = ESP_UUID_LEN_16;
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[FORCE_OTA_ID].char_uuid.uuid.uuid16 = FORCE_OTA_CHAR_UUID;
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[FORCE_OTA_ID].property = SYSTEM_SETTINGS_SERVICE_PROPERTY;
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[FORCE_OTA_ID].perm = ESP_GATT_PERM_READ| ESP_GATT_PERM_WRITE;
    
    // Force OTA CHARACTERISTIC       
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[CANCEL_OTA_ID].char_uuid.len = ESP_UUID_LEN_16;
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[CANCEL_OTA_ID].char_uuid.uuid.uuid16 = CANCEL_OTA_CHAR_UUID;
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[CANCEL_OTA_ID].property = SYSTEM_SETTINGS_SERVICE_PROPERTY;
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[CANCEL_OTA_ID].perm = ESP_GATT_PERM_READ| ESP_GATT_PERM_WRITE;
 
    //  Connect Wifi CHARACTERISTIC       
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[CONNECT_WIFI_ID].char_uuid.len = ESP_UUID_LEN_16;
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[CONNECT_WIFI_ID].char_uuid.uuid.uuid16 = CONNECT_WIFI_CHAR_UUID;
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[CONNECT_WIFI_ID].property = SYSTEM_SETTINGS_SERVICE_PROPERTY;
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[CONNECT_WIFI_ID].perm = ESP_GATT_PERM_READ| ESP_GATT_PERM_WRITE;  
   
    //  Software Version Characteristic       
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[SOFTWARE_VERSION_ID].char_uuid.len = ESP_UUID_LEN_16;
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[SOFTWARE_VERSION_ID].char_uuid.uuid.uuid16 = SOFTWARE_VERSION_CHAR_UUID;
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[SOFTWARE_VERSION_ID].property = SYSTEM_SETTINGS_SERVICE_PROPERTY;
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[SOFTWARE_VERSION_ID].perm = ESP_GATT_PERM_READ;  
    
    //ADD DeviceID  characteristic
    esp_ble_gatts_add_char(gatts_profile_array[SYSTEM_SETTINGS_SERVICE].service_handle, 
                               &gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[DEVICEID_ID].char_uuid,
                               gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[DEVICEID_ID].perm,
                               gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[DEVICEID_ID].property,
                               NULL, NULL);

    //ADD WiFi SSID  characteristic
    esp_ble_gatts_add_char(gatts_profile_array[SYSTEM_SETTINGS_SERVICE].service_handle, 
                               &gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[WIFI_SSID_ID].char_uuid,
                               gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[WIFI_SSID_ID].perm,
                               gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[WIFI_SSID_ID].property,
                               NULL, NULL);

    //ADD WiFi Password  characteristic
    esp_ble_gatts_add_char(gatts_profile_array[SYSTEM_SETTINGS_SERVICE].service_handle, 
                               &gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[WIFI_PASSWORD_ID].char_uuid,
                               gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[WIFI_PASSWORD_ID].perm,
                               gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[WIFI_PASSWORD_ID].property,
                               NULL, NULL);      
    //ADD OTA IP  characteristic
    esp_ble_gatts_add_char(gatts_profile_array[SYSTEM_SETTINGS_SERVICE].service_handle, 
                               &gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[OTA_IP_ID].char_uuid,
                               gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[OTA_IP_ID].perm,
                               gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[OTA_IP_ID].property,
                               NULL, NULL);                                                  

    //ADD OTA Port  characteristic
    esp_ble_gatts_add_char(gatts_profile_array[SYSTEM_SETTINGS_SERVICE].service_handle, 
                               &gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[OTA_PORT_ID].char_uuid,
                               gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[OTA_PORT_ID].perm,
                               gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[OTA_PORT_ID].property,
                               NULL, NULL);  

    //ADD OTA File Name  characteristic
    esp_ble_gatts_add_char(gatts_profile_array[SYSTEM_SETTINGS_SERVICE].service_handle, 
                               &gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[OTA_FILENAME_ID].char_uuid,
                               gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[OTA_FILENAME_ID].perm,
                               gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[OTA_FILENAME_ID].property,
                               NULL, NULL);  

    //ADD Do OTA characteristic
    esp_ble_gatts_add_char(gatts_profile_array[SYSTEM_SETTINGS_SERVICE].service_handle, 
                               &gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[FORCE_OTA_ID].char_uuid,
                               gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[FORCE_OTA_ID].perm,
                               gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[FORCE_OTA_ID].property,
                               NULL, NULL);
    //ADD Cancel OTA characteristic
    esp_ble_gatts_add_char(gatts_profile_array[SYSTEM_SETTINGS_SERVICE].service_handle, 
                               &gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[CANCEL_OTA_ID].char_uuid,
                               gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[CANCEL_OTA_ID].perm,
                               gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[CANCEL_OTA_ID].property,
                               NULL, NULL);
    // ADD Connect WiFi ID 
    esp_ble_gatts_add_char(gatts_profile_array[SYSTEM_SETTINGS_SERVICE].service_handle, 
                               &gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[CONNECT_WIFI_ID].char_uuid,
                               gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[CONNECT_WIFI_ID].perm,
                               gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[CONNECT_WIFI_ID].property,
                               NULL, NULL);
    // ADD Software Version ID
    esp_ble_gatts_add_char(gatts_profile_array[SYSTEM_SETTINGS_SERVICE].service_handle, 
                               &gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[SOFTWARE_VERSION_ID].char_uuid,
                               gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[SOFTWARE_VERSION_ID].perm,
                               gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[SOFTWARE_VERSION_ID].property,
                               NULL, NULL);

     esp_ble_gatts_start_service(gatts_profile_array[SYSTEM_SETTINGS_SERVICE].service_handle);

    case ESP_GATTS_ADD_CHAR_EVT: {
        ESP_LOGI(TAG,"ADD_CHAR_EVT, status %d,  attr_handle %x, service_handle %x, char uuid %x",
                param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle, param->add_char.char_uuid.uuid.uuid16);
        /* store characteristic handles for later usage */
        if(param->add_char.char_uuid.uuid.uuid16 == DEVICEID_CHAR_UUID){
            gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[DEVICEID_ID].char_handle = param->add_char.attr_handle;
        }
        else if(param->add_char.char_uuid.uuid.uuid16 == WIFI_SSID_CHAR_UUID){
            gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[WIFI_SSID_ID].char_handle = param->add_char.attr_handle;
        }
        else if(param->add_char.char_uuid.uuid.uuid16 == WIFI_PASSWORD_CHAR_UUID){
            gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[WIFI_PASSWORD_ID].char_handle = param->add_char.attr_handle;
        }
         else if(param->add_char.char_uuid.uuid.uuid16 == OTA_IP_CHAR_UUID){
            gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[OTA_IP_ID].char_handle = param->add_char.attr_handle;
        }
         else if(param->add_char.char_uuid.uuid.uuid16 == OTA_PORT_CHAR_UUID){
            gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[OTA_PORT_ID].char_handle = param->add_char.attr_handle;
        }
         else if(param->add_char.char_uuid.uuid.uuid16 == OTA_FILENAME_CHAR_UUID){
            gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[OTA_FILENAME_ID].char_handle = param->add_char.attr_handle;
        }
        else if(param->add_char.char_uuid.uuid.uuid16 == FORCE_OTA_CHAR_UUID){
            gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[FORCE_OTA_ID].char_handle = param->add_char.attr_handle;
        }
        else if(param->add_char.char_uuid.uuid.uuid16 == CANCEL_OTA_CHAR_UUID){
            gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[CANCEL_OTA_ID].char_handle = param->add_char.attr_handle;
        }
        else if(param->add_char.char_uuid.uuid.uuid16 == CONNECT_WIFI_CHAR_UUID){
            gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[CONNECT_WIFI_ID].char_handle = param->add_char.attr_handle;
        }
        else if(param->add_char.char_uuid.uuid.uuid16 == SOFTWARE_VERSION_CHAR_UUID){
            gatts_profile_array[SYSTEM_SETTINGS_SERVICE].chars[SOFTWARE_VERSION_ID].char_handle = param->add_char.attr_handle;
        }   
    }
    break;
    case ESP_GATTS_ADD_INCL_SRVC_EVT:
        break;
    case ESP_GATTS_DELETE_EVT:
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(TAG, "SERVICE_START_EVT, status %d, service_handle %d",
                 param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_STOP_EVT:
        break;
    case ESP_GATTS_CONNECT_EVT:
        ESP_LOGI(TAG, "CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x:",
                 param->connect.conn_id,
                 param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                 param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
        gatts_profile_array[SYSTEM_SETTINGS_SERVICE].conn_id = param->connect.conn_id;
        break;
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_CONF_EVT status %d", param->conf.status);
        if (param->conf.status != ESP_GATT_OK){
            esp_log_buffer_hex(TAG, param->conf.value, param->conf.len);
        }
    break;
    case ESP_GATTS_DISCONNECT_EVT:
    case ESP_GATTS_OPEN_EVT:
    case ESP_GATTS_CANCEL_OPEN_EVT:
    case ESP_GATTS_CLOSE_EVT:
    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
    default:
        break;
    }
}

static void device_state_gatts_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    // esp_err_t err;
    switch (event) {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(TAG, "REGISTER_APP_EVT, status %d, app_id %d", param->reg.status, param->reg.app_id);
        gatts_profile_array[DEVICE_STATE_SERVICE].service_id.is_primary = true;
        gatts_profile_array[DEVICE_STATE_SERVICE].service_id.id.inst_id = 0x00;
        gatts_profile_array[DEVICE_STATE_SERVICE].service_id.id.uuid.len = ESP_UUID_LEN_16;
        gatts_profile_array[DEVICE_STATE_SERVICE].service_id.id.uuid.uuid.uuid16 = DEVICE_STATE_SERVICE_UUID;
        current_gatts_if = gatts_if;
        setup_lead_advertising();
        esp_ble_gatts_create_service(gatts_if, &gatts_profile_array[DEVICE_STATE_SERVICE].service_id, NUM_HANDLE_DEVICE_STATE_SERVICE);
        break;

    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d", param->create.status, param->create.service_handle);
        gatts_profile_array[DEVICE_STATE_SERVICE].service_handle = param->create.service_handle;        
        DEVICE_STATE_SERVICE_PROPERTY = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE ;

    //WEIGHT DATA CHARACTERISTIC       
        gatts_profile_array[DEVICE_STATE_SERVICE].chars[WEIGHT_DATA_ID].char_uuid.len = ESP_UUID_LEN_16;
        gatts_profile_array[DEVICE_STATE_SERVICE].chars[WEIGHT_DATA_ID].char_uuid.uuid.uuid16 = WEIGHT_DATA_CHAR_UUID;
        gatts_profile_array[DEVICE_STATE_SERVICE].chars[WEIGHT_DATA_ID].property = DEVICE_STATE_SERVICE_PROPERTY;
        gatts_profile_array[DEVICE_STATE_SERVICE].chars[WEIGHT_DATA_ID].perm = ESP_GATT_PERM_READ| ESP_GATT_PERM_WRITE;

   
    // ADD WEIGHT DATA characteristic
    esp_ble_gatts_add_char(gatts_profile_array[DEVICE_STATE_SERVICE].service_handle, &gatts_profile_array[DEVICE_STATE_SERVICE].chars[WEIGHT_DATA_ID].char_uuid,
                               gatts_profile_array[DEVICE_STATE_SERVICE].chars[WEIGHT_DATA_ID].perm,
                               gatts_profile_array[DEVICE_STATE_SERVICE].chars[WEIGHT_DATA_ID].property,
                               NULL, NULL);

    esp_ble_gatts_start_service(gatts_profile_array[DEVICE_STATE_SERVICE].service_handle);

    case ESP_GATTS_ADD_CHAR_EVT: {
        ESP_LOGI(TAG,"ADD_CHAR_EVT, status %d,  attr_handle %x, service_handle %x, char uuid %x",
                param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle, param->add_char.char_uuid.uuid.uuid16);
        /* store characteristic handles for later usage */
        if(param->add_char.char_uuid.uuid.uuid16 == WEIGHT_DATA_CHAR_UUID){
            gatts_profile_array[DEVICE_STATE_SERVICE].chars[WEIGHT_DATA_ID].char_handle = param->add_char.attr_handle;
        }
    }
    break;
    case ESP_GATTS_DELETE_EVT:
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(TAG, "SERVICE_START_EVT, status %d, service_handle %d",
                 param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_STOP_EVT:
        break;
    case ESP_GATTS_CONNECT_EVT:{
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
        gatts_profile_array[DEVICE_STATE_SERVICE].conn_id = param->connect.conn_id;
        //start sent the update connection parameters to the peer device.
        esp_ble_gap_update_conn_params(&conn_params);
        esp_ble_gap_start_advertising(&adv_params);
        break;
    }
    case ESP_GATTS_READ_EVT: {
    ESP_LOGI(TAG, "GATT_READ_EVT, conn_id %d, trans_id %d, handle %d", param->read.conn_id, param->read.trans_id, param->read.handle);
        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
        rsp.attr_value.handle = param->read.handle;
        rsp.attr_value.len = 5;
        uint16_t currentHandle = param->read.handle;
        
        if (currentHandle == gatts_profile_array[DEVICE_STATE_SERVICE].chars[WEIGHT_DATA_ID].char_handle) {
            memcpy(rsp.attr_value.value, TotalmassStr, 5);
            ESP_LOGI(TAG, "Totalmass = %s", rsp.attr_value.value);
            // free(TotalmassStr);

        }
       
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                    ESP_GATT_OK, &rsp);
        break;
    }
    // case ESP_GATTS_WRITE_EVT: {
    //     ESP_LOGI(TAG, "Device State: GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d", param->write.conn_id, param->write.trans_id, param->write.handle);
    //     if (!param->write.is_prep){
    //         esp_log_buffer_hex(TAG, param->write.value, param->write.len);
    //         uint16_t writeHandle = param->write.handle;

            
    //             if (writeHandle ==  gatts_profile_array[DEVICE_STATE_SERVICE].chars[WEIGHT_DATA_ID].char_handle){
    //             int new_light_mode = atoi((char *) param->write.value);
    //             set_light_mode(new_light_mode);
    //             }
    //             else if (writeHandle == gatts_profile_array[DEVICE_STATE_SERVICE].chars[SESSION_TIME_ID].char_handle) {
    //                 int new_session_time = atoi((char *) param->write.value);
    //                 set_session_time(new_session_time);
    //             }
    //             else if (writeHandle == gatts_profile_array[DEVICE_STATE_SERVICE].chars[RUN_STATE_ID].char_handle) {
    //                 int new_run_state = atoi((char *) param->write.value);
    //                 set_run_state(new_run_state);
    //             }
    //             else if (writeHandle == gatts_profile_array[DEVICE_STATE_SERVICE].chars[REMAINING_TIME_ID].char_handle) {
    //                 int new_remaining_time = atoi((char *) param->write.value);
    //                 set_remaining_time(new_remaining_time);
    //             }
    //         }
    //     else {
    //         ESP_LOGI(TAG, "Needs write prep?");
    //     }
    //     esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
    //     break;
    // }

    case ESP_GATTS_EXEC_WRITE_EVT:
        ESP_LOGI(TAG,"ESP_GATTS_EXEC_WRITE_EVT");
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        break;
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
        break;
    case ESP_GATTS_UNREG_EVT:
        break;
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_DISCONNECT_EVT");
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_CONF_EVT, status %d", param->conf.status);
        if (param->conf.status != ESP_GATT_OK){
            esp_log_buffer_hex(TAG, param->conf.value, param->conf.len);
        }
        break;
    case ESP_GATTS_OPEN_EVT:
    case ESP_GATTS_CANCEL_OPEN_EVT:
    case ESP_GATTS_CLOSE_EVT:
    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
   
    break;
    default:
    break;
    }
}

esp_err_t enable_ble_lead()
{
    esp_err_t err;

    err = esp_ble_gatts_register_callback(lead_gatts_setup_handler);
    if (err)
    {
        ESP_LOGE(TAG, "Could not register lead gatts handler, error code = %x\n", err);
        return err;
    }

    err = setup_lead_advertising();
    if (err)
    {
        ESP_LOGE(TAG, "Could not set up lead advertising parameters, error code = %x\n", err);
        return err;
    }

    err = esp_ble_gap_register_callback(lead_gap_setup_handler);
    if (err)
    {
        ESP_LOGE(TAG, "Could not register lead gap handler, error code = %x\n", err);
        return err;
    }
    err = esp_ble_gatts_app_register(DEVICE_STATE_SERVICE);
    if (err)
    {
        ESP_LOGE(TAG, "Could not register Device State Service, error code = %x\n", err);
    }

    err = esp_ble_gatts_app_register(SYSTEM_SETTINGS_SERVICE);
    if (err)
    {
        ESP_LOGE(TAG, "Could not register System Settings Service, error code = %x\n", err);
    }
    return err;
}

esp_err_t disable_ble_lead()
{
    esp_err_t err;
    esp_ble_gap_stop_advertising();
    err = esp_ble_gatts_app_unregister(current_gatts_if);
    if (err != ESP_OK){
        ESP_LOGE(TAG, "Could not shut down lead mode, error code = %x\n", err);
    }
    return err;
}

esp_err_t setup_lead_advertising()
{
    esp_err_t err;
    // The hex and string representations WILL look different. They are both unique to each device.
    // We choose to use the string representation as it is easier to handle
    ESP_LOGI(TAG, "Adding Device ID to BLE Name: %s",device_id);
    char ble_adv_name[20] = DEFAULT_BLE_NAME;
    strcat(ble_adv_name, " ");
    strcat(ble_adv_name, device_id);
    err = ESP_OK;
    err = esp_ble_gap_set_device_name(ble_adv_name);
    if (err != ESP_OK){
        ESP_LOGE(TAG, "set device name failed, error code = %x", err);
    }
    //config adv data
    err = esp_ble_gap_config_adv_data(&adv_data);
    if (err){
        ESP_LOGE(TAG, "config adv data failed, error code = %x", err);
    }
    adv_config_done |= adv_config_flag;

    // and add device state to manufacturer data
    // set_adv_device_state() is called anytime there is a change to device state
    err = set_adv_device_state();
    if (err){
        ESP_LOGE(TAG, "Could not copy device state to manufacturer data, error code = %x", err);
    }
    adv_config_done |= scan_rsp_config_flag;
    return err;
}



// END FUNCTION DEFINITIONS