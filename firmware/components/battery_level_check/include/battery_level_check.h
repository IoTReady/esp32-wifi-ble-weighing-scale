//
//  battery_level_check.h
//  Weighing_Scale_Firmware
//
//  ESP32 firmware for Smart Weighing Scale
//
//  This module provides battery monitoring behavior through ADC pins on ESP32.
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

#ifndef __BATTERY_LEVEL_CHECK_H__
#define __BATTERY_LEVEL_CHECK_H__ 1

// #define BATTERY_INDICATOR_1_GPIO_NUM 19
// #define BATTERY_INDICATOR_2_GPIO_NUM 18
// #define BATTERY_INDICATOR_3_GPIO_NUM 5

#define BATTERY_CHARGING_INDICATION_GPIO_NUM 23

TaskHandle_t battery_level_check_task_handle;

void battery_level_check_task(void *pvParameter);
void battery();

extern uint32_t battery_adc_reading;
extern int battery_level;
extern int charging_state;
bool displaying_batt_low;


#endif // __BATTERY_LEVEL_CHECK_H__
