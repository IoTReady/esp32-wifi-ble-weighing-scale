//
//  common.c
//  Weighing_Scale_Firmware
//
//  ESP32 firmware for Smart Weighing Scale
//
//  This module provides functions and queues to pass data between other modules.
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

#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_log.h"
#include "common.h"
#include "ble_lead.h"
#include "wifi_sta.h"
#include "nvs_store.h"

#define TAG "COMMON"


char device_id[13] = "000000000000";
int boot_mode = 0; // 0 = normal, 1 = OTA
float Totalmass;
char TotalmassStr[16] ="000000000000000";

// TaskHandle_t weighingTask_Handle, tareTask_Handle;


wifi_sta_init_struct_t wifi_params = {
    .network_ssid = DEFAULT_WIFI_SSID,
    .network_password = DEFAULT_WIFI_PASSWORD
};

void set_device_id()
{
    uint8_t chipid[6];
    esp_efuse_mac_get_default(chipid);
    //   ESP_LOGI(TAG,"Chip ID in hex: %x%x%x%x%x%x\n", chipid[0], chipid[1], chipid[2], chipid[3], chipid[4], chipid[5]);
    sprintf(device_id,"%02x%02x%02x%02x%02x%02x",chipid[0], chipid[1], chipid[2], chipid[3], chipid[4], chipid[5]);
}

