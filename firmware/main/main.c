//
//  main.c
//  Weighing_Scale_Firmware
//
//  ESP32 firmware for Smart Weighing Scale
// 
//  This is the main file for Weighing_Scale_Firmware.
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

#include "HX711.h"
#include "weighing_scale.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "console_lib.h"
#include "ble_lead.h"
#include "ble_common.h"
#include "common.h"
#include "esp_log.h"
#include "nvs_store.h"
#include "esp_event_loop.h"
#include "driver/gpio.h"
#include "battery_level_check.h"

#define TAG "main"

// #define BUTTON_GPIO 19


static esp_err_t app_event_handler(void *ctx, system_event_t *event);

void app_main()
{   
    esp_err_t err;

    /* Get the device MAC ID */
    set_device_id();

    vTaskDelay(100/portTICK_PERIOD_MS);

    /* Initialize Non-volatile Storage */
    init_nvs();

    // Init Display and display "HELLO . . ." until weighing scale init is completed
    initDisplay();

    /* WiFi */
    // err = read_wifi_params_nvs();
    // if (err)
    // {
    //     write_wifi_params_nvs();
    // }

    /* Initialize Wi-Fi event handler */    
    esp_event_loop_init(&app_event_handler, NULL);

    /* Initialize BLE */
    init_ble();

    /* Register BLE characteristics, services and callbacks */    
    enable_ble_lead();

    vTaskDelay(100/portTICK_PERIOD_MS);
    
    /* Initialize Weighing Scale */
    init_weighingScale();

    /* Set value to zero on initialization and write it to NVS */
    tare(5);
    write_offset_nvs();

    /* Check battery level at initialization */
    battery();

    /* Initialize console UART to tare and calibrate through keyboard */    
    initialize_console();

    ESP_LOGI(TAG,"OFFSET: %ld \n", OFFSET);
    ESP_LOGI(TAG,"SCALE: %ld \n", SCALE);
    
    /* Initialize weighing task */
    xTaskCreatePinnedToCore(&weighingTask, "weighingTask", 8192, NULL, 7, &weighingTask_Handle, APP_CPU_NUM);
    /* Watch tare button to tare weight to zero */
    xTaskCreate(&tareTask, "tareTask", 3 * 1024, NULL, 5, &tareTask_Handle);
    /* Initialize task to monitor battery level */
    xTaskCreate(&battery_level_check_task, "battery_level_check_task", 3 * 1024, NULL, LOW_PRIORITY, &battery_level_check_task_handle);
    /* Initialize console UART task for calibration and Tare */
    xTaskCreate(&run_console, "run_console", 4 * 1024, NULL, 5, NULL);


    // wifi_sta_init(&wifi_params);
    
}


/* Wi-Fi event handler */
static esp_err_t app_event_handler(void *ctx, system_event_t *event)
{
    esp_err_t result = ESP_OK;
    int handled = 0;
    
    ESP_LOGI(TAG, "app_event_handler: event: %d", event->event_id);

    /* Let the wifi_sta module handle all WIFI STA events */
    
    result = wifi_sta_handle_event(ctx, event, &handled);
    if (ESP_OK != result || handled) {
        return result;
    }
        
    ESP_LOGW(TAG, "app_event_handler: unhandled event: %d", event->event_id);
    return ESP_OK;
}