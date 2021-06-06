//  weighing_scale.c
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

#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_log.h"
#include "weighing_scale.h"
#include "HX711.h"
#include <string.h>
#include "driver/gpio.h"
#include "console_lib.h"
#include "nvs_store.h"
#include "common.h"
#include <max7219.h>
#include <battery_level_check.h>


#define DELAY 2000


#define HOST HSPI_HOST

#define PIN_NUM_MOSI 12
#define PIN_NUM_CLK  14
#define PIN_NUM_CS   13


bool weighing_flag = false, tare_flag = false, displaying_batt_low = false;


#define CHECK(expr, msg) \
    while ((res = expr) != ESP_OK) { \
        printf(msg "\n", res); \
        vTaskDelay(250 / portTICK_RATE_MS); \
    }

#define TAG "Weighing_scale"

// Configure SPI bus
spi_bus_config_t cfg = {
    .mosi_io_num = PIN_NUM_MOSI,
    .miso_io_num = -1,
    .sclk_io_num = PIN_NUM_CLK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 0,
    .flags = 0
};

// Configure display device
max7219_t dev = {
   .cascade_size = 1,
   .digits = 8,
   .mirrored = true
};

esp_err_t res;


void init_weighingScale()
{

    ESP_LOGI(TAG,"Initialising Weighing Scale");
    begin(DOUT, PD_SCK, 128);
    read_offset_nvs(&OFFSET);
    read_scale_nvs(&SCALE);
    ESP_LOGI(TAG,"NVS OFFSET: %ld\n", OFFSET);
    ESP_LOGI(TAG,"NVS SCALE: %ld\n", SCALE);

}

void initDisplay()
{

    CHECK(spi_bus_initialize(HOST, &cfg, 1),
            "Could not initialize SPI bus: %d");

    CHECK(max7219_init_desc(&dev, HOST, PIN_NUM_CS),
            "Could not initialize MAX7129 descriptor: %d");

    CHECK(max7219_init(&dev),
            "Could not initialize MAX7129: %d");

    max7219_set_brightness(&dev, 7);

    max7219_draw_text_7seg(&dev, 0, "HELL0 . . .");

}

void display_tare()
{
    max7219_draw_text_7seg(&dev, 0, " tare . . . .");
}

void display_clear()
{
    max7219_clear(&dev);
}

void tareTask(void *pvParameter) {
    gpio_pad_select_gpio(BUTTON_GPIO);
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_GPIO, GPIO_PULLUP_ONLY);
    while(1)
    {
        vTaskDelay(10/portTICK_PERIOD_MS);        
        if ( gpio_get_level(BUTTON_GPIO) == 0 )
        {
            vTaskDelay(100/portTICK_PERIOD_MS);
            tare_flag = true;
        }
    }
}

void display_percentage(int batt_level)
{
    switch(batt_level)
        {
            case 3:
            {
                printf("CASE 3\n");
                max7219_draw_text_7seg(&dev, 0, "BATT HI ");
                break;
            }
            case 2:
            {
                printf("CASE 2\n");
                max7219_draw_text_7seg(&dev, 0, "BATT MED");         
                break;
            }
            case 1:
            {
                printf("CASE 1\n");
                max7219_draw_text_7seg(&dev, 0, "BATT MED");         
                break;
            }
            case 0:
            {
                printf("CASE 0\n");
                max7219_draw_text_7seg(&dev, 0, "BATT LO ");
                break;
            }
        }
    vTaskDelay(1000 / portTICK_PERIOD_MS );
}

void weighingTask(void *pvParameter)
{

    vTaskDelay(1000 / portTICK_PERIOD_MS );

    max7219_clear(&dev);

    weighing_flag = true;
    
    while(1)
    {
        float mass = get_units(1);
        Totalmass = mass;
        char DisplaymassStr[16];
        ESP_LOGI(TAG,"mass = %.3f\n", mass);
        sprintf(TotalmassStr, "%2.3f", Totalmass);

        if(Totalmass <= 0.005 && Totalmass >= -0.005)
            Totalmass = 0;
        
        float Displaymass = mass;

        if(displaying_batt_low == false)
        {
        if(Totalmass < 0)
        {
            Displaymass *= -1;
            sprintf(DisplaymassStr, "%2.5f", Displaymass);
            max7219_set_digit(&dev, 3, 0x01);       // Display minus
            max7219_draw_text_7seg(&dev, 4, DisplaymassStr);
        }
        else if(Totalmass == 0)
        {
            max7219_set_digit(&dev, 3, 0);
            max7219_set_digit(&dev, 4, 0);
            max7219_set_digit(&dev, 5, 0);
            max7219_set_digit(&dev, 6, 0);
            max7219_draw_text_7seg(&dev, 7, "0");
        }
        else if(Totalmass < 10.00000)
        {
            sprintf(DisplaymassStr, "%2.5f", Displaymass);                        
            max7219_set_digit(&dev, 3, 0);
            max7219_draw_text_7seg(&dev, 4, DisplaymassStr);
        }
        else
        {
            sprintf(DisplaymassStr, "%2.5f", Displaymass);                    
            max7219_draw_text_7seg(&dev, 3, DisplaymassStr);
        }
        }
        
        
        vTaskDelay(200 / portTICK_PERIOD_MS );
        if (tare_flag == true)
        {
            display_tare();
            tare(2);
            vTaskDelay(100 / portTICK_PERIOD_MS );
            tare_flag = false;
            display_clear();              
        }

    }

}