//
//  iap.h
//  Weighing_Scale_Firmware
//
//  ESP32 firmware for Smart Weighing Scale
// 
//  This module is responsible for writing the firmware to the flash.
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


#ifndef __IAP__
#define __IAP__ 1


typedef int32_t iap_err_t;

#define IAP_OK      0
#define IAP_FAIL    -1
#define IAP_ERR_ALREADY_INITIALIZED     0x101
#define IAP_ERR_NOT_INITIALIZED         0x102
#define IAP_ERR_SESSION_ALREADY_OPEN    0x103
#define IAP_ERR_OUT_OF_MEMORY           0x104
#define IAP_ERR_NO_SESSION              0x105
#define IAP_ERR_PARTITION_NOT_FOUND     0x106
#define IAP_ERR_WRITE_FAILED            0x107


// Call once at application startup, before calling any other function of this module.
iap_err_t iap_init();

// Call to start a programming session.
// Sets the programming pointer to the start of the next OTA flash partition.
iap_err_t iap_begin();

// Call to write a block of data to the current location in flash.
// If the write fails, you need to abort the current programming session
// with 'iap_abort' and start again from the beginning.
iap_err_t iap_write(uint8_t *bytes, uint16_t len);

// Call to close a programming session and activate the programmed partition.
iap_err_t iap_commit();

// Abort the current programming session.
iap_err_t iap_abort();


#endif // __IAP__
