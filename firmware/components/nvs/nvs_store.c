//
//  nvs_store.c
//  Weighing_Scale_Firmware
//
//  ESP32 firmware for Smart Weighing Scale
//
//  This module provides functions to store and read settings from NVS.
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

#include "nvs_flash.h"
#include "esp_log.h"
#include "HX711.h"
#include "wifi_sta.h"
#include "common.h"
// END #INCLUDES

// START #DEFINES

#define TAG "nvs_store"
#define STORAGE_NAMESPACE "storage"
#define WIFI_PARAMS_STORE "settings"
#define USER_PREFERENCES_STORE "preferences"
#define LIGHT_MODE_STORE "light_mode"
#define SESSION_TIME_STORE "session_time"
#define LEAD_ID_STORE "lead_id"
#define BOOT_MODE_STORE "boot_mode"
#define SCALE_STORE "scale_store"
#define OFFSET_STORE "offset_store"

// END #DEFINES

// START MODULE GLOBAL VARIABLES


// END MODULE GLOBAL VARIABLES

// START FUNCTION DECLARATIONS

esp_err_t init_nvs();
esp_err_t write_wifi_params_nvs();
esp_err_t read_wifi_params_nvs();
esp_err_t write_offset_nvs();
esp_err_t write_scale_nvs();
esp_err_t read_offset_nvs(int *new_offset);
esp_err_t read_scale_nvs(int *new_scale_store);

// END FUNCTION DECLARATIONS

// START FUNCTION DEFINITIONS

// Initialise NVS and open storage for use
// We should call nvs_close before shutting down to avoid corruption
esp_err_t init_nvs()
{
    static esp_err_t err;
    ESP_LOGI(TAG, "Initialising NVS");
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    return err;
}


esp_err_t write_wifi_params_nvs()
{
    
    static nvs_handle nvsHandle;
    static esp_err_t err;
    nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvsHandle);
    size_t required_size = sizeof(wifi_sta_init_struct_t);
    nvs_set_blob(nvsHandle, WIFI_PARAMS_STORE, &wifi_params, required_size);
    err = nvs_commit(nvsHandle);
    nvs_close(nvsHandle);
    
    return err;
}

esp_err_t read_wifi_params_nvs()
{
    
    static nvs_handle nvsHandle;
    static esp_err_t err;
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvsHandle);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    }
    else 
    {
        // Read the size of memory space required for struct
        size_t required_size = 0;  // value will default to 0, if not set yet in NVS
        err = nvs_get_blob(nvsHandle, WIFI_PARAMS_STORE, NULL, &required_size);
        // ESP_LOGI(TAG, "Retrieved size = %d and actual size = %d", required_size, sizeof(system_settings_t));

        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) 
        {
            ESP_LOGE(TAG, "Error (%s) while reading WiFi params struct size.", esp_err_to_name(err));
        }
        else if (required_size != sizeof(wifi_sta_init_struct_t))
        {
            ESP_LOGE(TAG, "WiFi params not found. Perhaps not stored yet?");
            err = -1;
        }
        else
        {
            // Read previously saved blob if available
            err = nvs_get_blob(nvsHandle, WIFI_PARAMS_STORE, &wifi_params, &required_size);
            if (err != ESP_OK) 
            {
                ESP_LOGE(TAG, "Error (%s) while reading WiFi params.", esp_err_to_name(err));
            }
        }
    }
    nvs_close(nvsHandle);
    
    return err;
}

esp_err_t write_offset_nvs()
{
    
    static nvs_handle nvsHandle;
    static esp_err_t err;
    nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvsHandle);
    nvs_set_i32(nvsHandle, OFFSET_STORE, OFFSET);
    err = nvs_commit(nvsHandle);
    nvs_close(nvsHandle);
    
    return err;
}

esp_err_t write_scale_nvs()
{
    
    static nvs_handle nvsHandle;
    static esp_err_t err;
    nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvsHandle);
    nvs_set_i32(nvsHandle, SCALE_STORE, SCALE);
    err = nvs_commit(nvsHandle);
    nvs_close(nvsHandle);
    
    return err;
}

esp_err_t read_offset_nvs(int *new_offset)
{
    
    static nvs_handle nvsHandle;
    static esp_err_t err;
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvsHandle);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    }
    else 
    {
        err = nvs_get_i32(nvsHandle, OFFSET_STORE, new_offset);
        if (err != ESP_OK) 
        {
            ESP_LOGE(TAG, "Error (%s) while reading light mode.", esp_err_to_name(err));
        }
    }
    nvs_close(nvsHandle);
    
    return err;
}

esp_err_t read_scale_nvs(int *new_scale_store)
{
    
    static nvs_handle nvsHandle;
    static esp_err_t err;
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvsHandle);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    }
    else 
    {
        err = nvs_get_i32(nvsHandle, SCALE_STORE, new_scale_store);
        if (err != ESP_OK) 
        {
            ESP_LOGE(TAG, "Error (%s) while reading scale.", esp_err_to_name(err));
        }
    }
    nvs_close(nvsHandle);
    
    return err;
}


// END FUNCTION DEFINITIONS