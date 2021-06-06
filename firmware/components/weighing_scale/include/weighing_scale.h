//
//  weighing_scale.h
//  Weighing_Scale_Firmware
//
//  ESP32 firmware for Smart Weighing Scale
//
//  This module provides control to read weight data.
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

#ifndef __WEIGHING_SCALE_H__
#define __WEIGHING_SCALE_H__ 1


#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "HX711.h"

TaskHandle_t weighingTask_Handle, tareTask_Handle;

#define DOUT 25
#define PD_SCK 26

#define BUTTON_GPIO 19


// functions
void init_weighingScale();
void initDisplay();
void display_percentage(int batt_level);
void display_tare();
void display_clear();

void weighingTask(void *pvParameter);
void tareTask(void *pvParameter);


#endif // __COMMON_H__
