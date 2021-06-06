//
//  nvs_store.h
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

#ifndef __NVS_STORE_H__
#define __NVS_STORE_H__ 1

#include "esp_err.h"
// #include "common.h"

esp_err_t init_nvs();
esp_err_t write_wifi_params_nvs();
esp_err_t read_wifi_params_nvs();
esp_err_t write_offset_nvs();
esp_err_t read_offset_nvs(int *offset_store);
esp_err_t read_scale_nvs(int *scale_store);
esp_err_t write_scale_nvs();

#endif // __NVS_STORE_H__
