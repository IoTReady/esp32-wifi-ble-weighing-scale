//
//  ble_common.h
//  Weighing_Scale_Firmware
//
//  ESP32 firmware for Smart Weighing Scale
//
//  This module provides common definitions for use by all BLE modules.
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

#ifndef __BLE_COMMON_H__
#define __BLE_COMMON_H__ 1

#include "esp_gap_ble_api.h"

esp_ble_adv_data_t adv_data;
esp_ble_adv_data_t scan_rsp_data;

esp_err_t init_ble();
esp_err_t set_adv_device_state();

#endif // __BLE_COMMON_H__
