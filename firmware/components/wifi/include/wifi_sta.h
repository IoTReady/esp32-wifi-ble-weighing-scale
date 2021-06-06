//
//  wifi_sta.h
//  Weighing_Scale_Firmware
//
//  ESP32 firmware for Smart Weighing Scale
// 
//  This module is responsible for establishing and maintaining the
//  WIFI connection to the defined access point.
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

#ifndef __WIFI_STA__
#define __WIFI_STA__ 1

#include "esp_event.h"
#include "freertos/event_groups.h"

#define WIFI_STA_EVENT_GROUP_CONNECTED_FLAG (1 << 0)


typedef struct wifi_sta_init_struct_ {
    
    // Network SSID to connect to.
    char network_ssid[32];
    
    // Network password.
    char network_password[32];
    
} wifi_sta_init_struct_t;

void disable_wifi();
void enable_wifi();
void toggle_wifi();

// Configure this device in 'station' mode and connect to the specified network.
esp_err_t wifi_sta_init(wifi_sta_init_struct_t *param);

// Sets "handled" to 1 if the event was handled, 0 if it was not for us.
esp_err_t wifi_sta_handle_event(void *ctx, system_event_t *event, int *handled);

// Returns 1 if the device is currently connected to the specified network, 0 otherwise.
int wifi_sta_is_connected();

// Let other modules wait on connectivity changes.
EventGroupHandle_t wifi_sta_get_event_group();




#endif // __WIFI_STA__
