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


#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"
#include "battery_level_check.h"
#include "weighing_scale.h"


static const char *TAG = "battery_level";

#define DEFAULT_VREF 1100 //Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES 64  //Multisampling
#define TAG "battery_level_check"


#define BATT_THRESHOLD 1200
#define BATT_MIN 1000

static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t channel = ADC_CHANNEL_6; //GPIO34 if ADC1, GPIO14 if ADC2
// static const adc_channel_t channel = ADC_CHANNEL_4; //GPIO32 if ADC1, GPIO13 if ADC2
static const adc_atten_t atten = ADC_ATTEN_DB_11;
static const adc_unit_t unit = ADC_UNIT_1;
uint32_t battery_adc_reading;
uint32_t battery_voltage;
int battery_level;
int charging_state = 0;

TaskHandle_t battery_level_check_task_handle;

static void check_efuse()
{
    /* We are using default Vref i.e. 1100mV */
    //Check TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK)
    {
        // printf("eFuse Two Point: Supported\n");
    }
    else
    {
        // printf("eFuse Two Point: NOT supported\n");
    }

    //Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK)
    {
        // printf("eFuse Vref: Supported\n");
    }
    else
    {
        // printf("eFuse Vref: NOT supported\n");
    }
}

static void print_char_val_type(esp_adc_cal_value_t val_type)
{
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP)
    {
        // printf("Characterized using Two Point Value\n");
    }
    else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF)
    {
        // printf("Characterized using eFuse Vref\n");
    }
    else
    {
        printf("Characterized using Default Vref\n");
    }
}

int get_gpio_state(int GPIO_NUM)
{
    int state;
    esp_err_t err;
    gpio_pad_select_gpio(GPIO_NUM);
    err = gpio_set_direction(GPIO_NUM, GPIO_MODE_INPUT);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not set GPIO NUM %d as Input", GPIO_NUM);
        return err;
    }
    state = gpio_get_level(GPIO_NUM);
    // ESP_LOGI(TAG, "GPIO PIN NUM %d state = %d", GPIO_NUM, state);
    return state;
}

/* Function to check battery at startup */
void battery()
{
    //Check if Two Point or Vref are burned into eFuse
    check_efuse();

    //Configure ADC
    if (unit == ADC_UNIT_1)
    {
        adc1_config_width(ADC_WIDTH_BIT_12);
        adc1_config_channel_atten(channel, atten);
    }
    else
    {
        adc2_config_channel_atten((adc2_channel_t)channel, atten);
    }

    //Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
    print_char_val_type(val_type);

    // uint32_t adc_reading = 0;
        int adc_reading = 0;
        // vTaskSuspend(weighingTask_Handle);
        //Multisampling
        for (int i = 0; i < NO_OF_SAMPLES; i++)
        {
            if (unit == ADC_UNIT_1)
            {
                adc_reading += adc1_get_raw((adc1_channel_t)channel);
            }
            else
            {
                int raw;
                adc2_get_raw((adc2_channel_t)channel, ADC_WIDTH_BIT_12, &raw);
                adc_reading += raw;
            }
        }
        adc_reading /= NO_OF_SAMPLES;
        //Convert adc_reading to voltage in mV
        battery_voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
        // vTaskResume(weighingTask_Handle);
        ESP_LOGI(TAG,"BATTERY LEVEL ADC: %d\n", adc_reading);
        ESP_LOGI(TAG,"BATTERY LEVEL VOLTAGE: %d\n", battery_voltage);
        
        printf("Adc Reading: %d\n", adc_reading);

        /* For battery percentage calculation */ 
        // int batt_percent = 1;
        // batt_percent = ((adc_reading - 1000) / BATT_THRESHOLD) * 100;
        // batt_percent = 100;
        // printf("Batt_percent = %d\n", batt_percent);
        // display_percentage(batt_percent);
        // if(adc_reading >= 1950)
        // {
        //     battery_level = 3;
        // }
        // else if(adc_reading >= 1800)
        // {
        //     battery_level = 2;
        // }
        // else if(adc_reading >= 1600)
        // {
        //     battery_level = 1;
        // }

        int new_charging_state = get_gpio_state(BATTERY_CHARGING_INDICATION_GPIO_NUM);
        if (new_charging_state == 1)
        {
            // ESP_LOGI(TAG, "Charging at level %d", battery_level);
            charging_state = 1;
        }
        else if (new_charging_state == 0 && charging_state == 1)
        {
            // ESP_LOGI(TAG, "Stopped charging at level %d", battery_level);
            charging_state = 0;
        }
        switch (new_charging_state)
        {
            case 0:
            {
                // if (adc_reading < 1850)
                if (adc_reading < 2150)
                {
                    battery_level = 0;
                }
                // else if (adc_reading >= 1850 && adc_reading < 2200)
                else if (adc_reading >= 2150 && adc_reading < 2650)
                {
                    battery_level = 1;
                }
                // else if (adc_reading >= 2200 && adc_reading < 2800)
                else if (adc_reading >= 2650 && adc_reading < 3100)
                {
                    battery_level = 2;
                }
                // else if (adc_reading >= 2800)
                else if (adc_reading >= 3100)
                {
                    battery_level = 3;
                }
                break;
            }
            case 1:
            {
                if (adc_reading < 2800)
                {
                    battery_level = 0;
                }
                else if (adc_reading >= 2800 && adc_reading < 2900)
                {
                    battery_level = 1;
                }
                else if (adc_reading >= 2900 && adc_reading < 3150)
                {
                    battery_level = 2;
                }
                else if (adc_reading >= 3150)
                {
                    battery_level = 3;
                }
                break;
            }
            if (battery_level < 0) {
                battery_level = 0;
            }
            // ESP_LOGI(TAG, "Charging at level %d", battery_level);
        }

        display_percentage(battery_level);
        // display_percentage(adc_reading);

}


void battery_level_check_task(void *pvParameter)
{
    /* setting up gpios for battery indication LEDs */
    // gpio_pad_select_gpio(BATT_INDICATOR_LED1);
    // gpio_pad_select_gpio(BATT_INDICATOR_LED2);
    // gpio_pad_select_gpio(BATT_INDICATOR_LED3);
    // gpio_set_direction(BATT_INDICATOR_LED1, GPIO_MODE_OUTPUT);
    // gpio_set_direction(BATT_INDICATOR_LED2, GPIO_MODE_OUTPUT);
    // gpio_set_direction(BATT_INDICATOR_LED3, GPIO_MODE_OUTPUT);

    //Check if Two Point or Vref are burned into eFuse
    check_efuse();

    //Configure ADC
    if (unit == ADC_UNIT_1)
    {
        adc1_config_width(ADC_WIDTH_BIT_12);
        adc1_config_channel_atten(channel, atten);
    }
    else
    {
        adc2_config_channel_atten((adc2_channel_t)channel, atten);
    }

    //Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
    print_char_val_type(val_type);

    //Continuously sample ADC1
    while (1)
    {
        // uint32_t adc_reading = 0;
        int adc_reading = 0;
        vTaskSuspend(weighingTask_Handle);
        //Multisampling
        for (int i = 0; i < NO_OF_SAMPLES; i++)
        {
            if (unit == ADC_UNIT_1)
            {
                adc_reading += adc1_get_raw((adc1_channel_t)channel);
            }
            else
            {
                int raw;
                adc2_get_raw((adc2_channel_t)channel, ADC_WIDTH_BIT_12, &raw);
                adc_reading += raw;
            }
        }
        adc_reading /= NO_OF_SAMPLES;
        //Convert adc_reading to voltage in mV
        battery_voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
        vTaskResume(weighingTask_Handle);
        ESP_LOGI(TAG,"BATTERY LEVEL ADC: %d\n", adc_reading);
        ESP_LOGI(TAG,"BATTERY LEVEL VOLTAGE: %d\n", battery_voltage);
        
        // int batt_percent = 1;
        int new_charging_state = get_gpio_state(BATTERY_CHARGING_INDICATION_GPIO_NUM);
        if (new_charging_state == 1)
        {
            ESP_LOGI(TAG, "Charging at level %d", battery_level);
            charging_state = 1;
        }
        else if (new_charging_state == 0 && charging_state == 1)
        {
            ESP_LOGI(TAG, "Stopped charging at level %d", battery_level);
            charging_state = 0;
        }
        switch (new_charging_state)
        {
            case 0:
            {
                if (adc_reading < 2150)
                {
                    battery_level = 0;
                }
                else if (adc_reading >= 2150 && adc_reading < 2650)
                {
                    battery_level = 1;
                }
                else if (adc_reading >= 2650 && adc_reading < 3100)
                {
                    battery_level = 2;
                }
                else if (adc_reading >= 3100)
                {
                    battery_level = 3;
                }
                break;
            }
            case 1:
            {
                if (adc_reading < 2800)
                {
                    battery_level = 0;
                }
                else if (adc_reading >= 2800 && adc_reading < 2900)
                {
                    battery_level = 1;
                }
                else if (adc_reading >= 2900 && adc_reading < 3150)
                {
                    battery_level = 2;
                }
                else if (adc_reading >= 3150)
                {
                    battery_level = 3;
                }
                break;
            }
            if (battery_level < 0) {
                battery_level = 0;
            }
        }

        ESP_LOGI(TAG, "BATTERY LEVEL %d", battery_level);

        // switch(battery_level)
        // {
            // case 3:
            // {
                // ESP_LOGI(TAG, "BATTERY LEVEL %d", battery_level);
                // printf("CASE 3\n");
                // gpio_set_level(BATT_INDICATOR_LED1, 1);
                // gpio_set_level(BATT_INDICATOR_LED2, 1);
                // gpio_set_level(BATT_INDICATOR_LED3, 1);
                // break;
            // }
            // case 2:
            // {
                // printf("CASE 2\n");
                // gpio_set_level(BATT_INDICATOR_LED1, 1);
                // gpio_set_level(BATT_INDICATOR_LED2, 1);
                // gpio_set_level(BATT_INDICATOR_LED3, 0);            
                // break;
            // }
            // case 1:
            // {
                // printf("CASE 1\n");
                // gpio_set_level(BATT_INDICATOR_LED1, 1);
                // gpio_set_level(BATT_INDICATOR_LED2, 0);
                // gpio_set_level(BATT_INDICATOR_LED3, 0);            
                // break;
            // }
            // case 0:
            if (battery_level == 0 && new_charging_state == 0)
            {
                // vTaskSuspend(weighingTask_Handle);
                displaying_batt_low = true;
                display_percentage(battery_level);
                vTaskDelay(300/ portTICK_PERIOD_MS);
                display_clear();
                displaying_batt_low = false;
                // vTaskResume(weighingTask_Handle);
            }
        // }
		vTaskDelay(5000/ portTICK_PERIOD_MS);
    }
}