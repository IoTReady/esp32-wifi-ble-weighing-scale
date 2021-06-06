//
//  HX711.c
//  Weighing_Scale_Firmware
//
//  ESP32 firmware for Smart Weighing Scale
// 
//  This module provides functions to configure the HX711 chip.
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


#include <HX711.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_log.h"
#include "console_lib.h"
#include "nvs_store.h"

#define TAG "HX711"


#define LSBFIRST 0
#define MSBFIRST 1

float scale = 1.0;
uint8_t times = 10;
	
long OFFSET = 0;
long SCALE = 1;

void yield(void) {};
 
void HX711( uint8_t dout, uint8_t pd_sck,  uint8_t gain) {
	begin(dout, pd_sck, 128);
}

void begin(uint8_t dout, uint8_t pd_sck, uint8_t gain) 
{

	PD_SCK = pd_sck;
	DOUT = dout;

	gpio_set_direction(PD_SCK, GPIO_MODE_OUTPUT);
    gpio_set_direction(DOUT, GPIO_MODE_INPUT);
	set_gain(gain);

}

bool is_ready() 
{

	return gpio_get_level(DOUT) == 0;

}

void printBits(uint8_t num)
{

   	uint8_t size = sizeof(num);
    uint8_t maxPow = 1<<(size*8-1);
    int i = 0;
    for(;i<size*8;++i)
	{
		// print last bit and shift left.
	    ESP_LOGI(TAG,"%u ",num&maxPow ? 1 : 0);
    	num = num<<1;
	}

}

void print32Bits(uint8_t num)
{

   	uint8_t size = sizeof(num);
    uint8_t maxPow = 1<<(size*8-1);
    // ESP_LOGI(TAG,"bit reading: %u\n",maxPow);
    int i = 0;
    for(;i<size*32;++i)
	{
		// print last bit and shift left.
		ESP_LOGI(TAG,"%u ",num&maxPow ? 1 : 0);
		num = num<<1;
	}

}

void set_gain(uint8_t gain)
{

	switch (gain)
	{
		case 128:		// channel A, gain factor 128
			GAIN = 1;
			break;
		case 64:		// channel A, gain factor 64
			GAIN = 3;
			break;
		case 32:		// channel B, gain factor 32
			GAIN = 2;
			break;
	}
    gpio_set_level(PD_SCK, 0);
	readValue();

}

uint8_t shiftIn(uint8_t dout, uint8_t pd_sck, uint8_t bitOrder)
{
    uint8_t i;
	// uint8_t *value [8];
	uint8_t value = 0;

	gpio_set_direction(PD_SCK, GPIO_MODE_OUTPUT);
	
    for(i = 0; i < 8; ++i) {
        gpio_set_level(PD_SCK, 1);

        if(bitOrder == LSBFIRST)
		{
    		// ESP_LOGI(TAG,"get value a");
            value |= gpio_get_level(DOUT) << i;
			// value[i] = gpio_get_level(DOUT);
        }
        else
		{
    		// ESP_LOGI(TAG,"get value b");
        	value |= gpio_get_level(DOUT) << (7 - i);
			// value[7 - i] = gpio_get_level(DOUT);
        }
		vTaskDelay(5/ portTICK_PERIOD_MS);
		gpio_set_level(PD_SCK, 0);
		vTaskDelay(10/ portTICK_PERIOD_MS);
    }
    // return (uint8_t) *value;	
    return value;	
} 

long readValue()
{

	// wait for the chip to become ready
	while (!is_ready()) {
		// Will do nothing on Arduino but prevent resets of ESP8266 (Watchdog Issue)
		yield();
	}

	u_long value = 0;
	uint8_t data[3] = { 0 };
	uint8_t filler = 0x00;

    ESP_LOGI(TAG,"reading value");

	// pulse the clock pin 24 times to readValue the data
	data[2] = shiftIn(DOUT, PD_SCK, MSBFIRST);
    // ESP_LOGI(TAG,"data[2] = %d",data[2]);
	// printBits(data[2]);

	data[1] = shiftIn(DOUT, PD_SCK, MSBFIRST);
    // ESP_LOGI(TAG,"data[1] = %d ",data[2]);
	// printBits(data[1]);

	data[0] = shiftIn(DOUT, PD_SCK, MSBFIRST);
    // ESP_LOGI(TAG,"data[0] = %d ",data[0]);
	// printBits(data[0]);

	// set the channel and the gain factor for the next reading using the clock pin
	for (uint8_t i = 0; i < GAIN; i++) {
		gpio_set_level(PD_SCK, 1);

        gpio_set_level(PD_SCK, 0);

    	ESP_LOGI(TAG,"GAIN = %d ",GAIN);
	}

	// Replicate the most significant bit to pad out a 32-bit signed integer
	if (data[2] & 0x80)
	{
		filler = 0xFF;
	}
	else 
	{
		filler = 0x00;
	}

	// Construct a 32-bit signed integer
	value = ( (u_long)(filler) << 24
			| (u_long)(data[2]) << 16
			| (u_long)(data[1]) << 8
			| (u_long)(data[0]) );

    // ESP_LOGI(TAG,"filler = %d ",filler);
    // ESP_LOGI(TAG,"data[2] = %d ",data[2]);
    // ESP_LOGI(TAG,"data[1] = %d ",data[1]);
    // ESP_LOGI(TAG,"data[0] = %d ",data[0]);
    // ESP_LOGI(TAG,"read value = %lu ",value);
	  ESP_LOGI(TAG,"Read Value: %lu \n", value);
	// print32Bits(value);
	return (long)(value);
	
}

long read_average( uint8_t times)
{

	long sum = 0;
	for ( uint8_t i = 0; i < times; i++) {
		sum += readValue();
		yield();
	}
	// sum = readValue();
	// return sum;
	
    ESP_LOGI(TAG,"average value = %ld", sum / times);
	return sum / times;

}

double get_value( uint8_t times)
{

	double val = read_average(times) - OFFSET;
    ESP_LOGI(TAG,"get value = %lf", val);

    // ESP_LOGI(TAG,"get value = %ld", read_average(times) - OFFSET);
	// return read_average(times) - OFFSET ;

	return val ;

}

float get_units( uint8_t times)
{

	float val = get_value(times) / SCALE;
    ESP_LOGI(TAG,"get units = %f", val);
    // ESP_LOGI(TAG,"get units = %f", get_value(times) / SCALE);

	// return get_value(times) / SCALE;
	return val;

}

void tare( uint8_t times)
{

	double sum = read_average(times);
	set_offset(sum);
	ESP_LOGI(TAG,"set offset: %lf \n", sum);

}

void set_scale(long scale) 
{

	SCALE = scale;

}

float get_scale()
{
	return SCALE;
}

void set_offset(long offset)
{
	
	OFFSET = offset;
	
}

long get_offset() 
{

	return OFFSET;

}

void power_down()
{
	
    gpio_set_level(PD_SCK, 0);
    gpio_set_level(PD_SCK, 1);
    
}

void power_up()
{
	
    gpio_set_level(PD_SCK, 0);

}
