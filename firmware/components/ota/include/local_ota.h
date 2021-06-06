//
//  local_ota.h
//  Weighing_Scale_Firmware
//
//  ESP32 firmware for Smart Weighing Scale
// 
//  This module is responsible for downloading firmware over the local
//  network and writing the firmware to the flash.
//  It manages the write buffer, writing to the flash, selecting the
//  correct partition and activating the partition.
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


#ifndef __LOCAL_OTA__
#define __LOCAL_OTA__ 1

extern TaskHandle_t *local_ota_task_handle;

void local_ota_task(void *pvParameter);


#endif // __LOCAL_OTA__
