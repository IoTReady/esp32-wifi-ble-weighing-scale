//
//  HX711.h
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

#ifndef HX711_h
#define HX711_h

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"


		uint8_t PD_SCK;	// Power Down and Serial Clock Input Pin
		uint8_t DOUT;		// Serial Data Output Pin
		uint8_t GAIN;		// amplification factor
		// long OFFSET;	// used for tare weight
		// float SCALE;	// used to return weight in grams, kg, ounces, whatever
		extern float scale;
		extern uint8_t times;
		
		extern long OFFSET;
		extern long SCALE;
		// define clock and data pin, channel, and gain factor
		// channel selection is made by passing the appropriate gain: 128 or 64 for channel A, 32 for channel B
		// gain: 128 or 64 for channel A; channel B works with 32 gain factor only
		void HX711(uint8_t dout, uint8_t pd_sck, uint8_t gain);

		// HX711();

		// virtual ~HX711();

		void tare_init(uint8_t times);

		// Allows to set the pins and gain later than in the constructor
		void begin(uint8_t dout, uint8_t pd_sck, uint8_t gain);

		// check if HX711 is ready
		// from the datasheet: When output data is not ready for retrieval, digital output pin DOUT is high. Serial clock
		// input PD_SCK should be low. When DOUT goes to low, it indicates data is ready for retrieval.
		bool is_ready();

		// set the gain factor; takes effect only after a call to readValue()
		// channel A can be set for a 128 or 64 gain; channel B has a fixed 32 gain
		// depending on the parameter, the channel is also set to either A or B
		void set_gain( uint8_t gain);

		// waits for the chip to be ready and returns a reading
		long readValue();

		// returns an average reading; times = how many times to readValue
		long read_average( uint8_t times);

		// returns (read_average() - OFFSET), that is the current value without the tare weight; times = how many readings to do
		double get_value( uint8_t times);

		// returns get_value() divided by SCALE, that is the raw value divided by a value obtained via calibration
		// times = how many readings to do
		float get_units( uint8_t times );

		// set the OFFSET value for tare weight; times = how many times to readValue the tare value
		void tare( uint8_t times);

		// set the SCALE value; this value is used to convert the raw data to "human readable" data (measure units)
		void set_scale(long scale);

		// get the current SCALE
		float get_scale();

		// set OFFSET, the value that's subtracted from the actual reading (tare weight)
		void set_offset(long offset);

		// get the current OFFSET
		long get_offset();

		// puts the chip into power down mode
		void power_down();

		// wakes up the chip after power down mode
		void power_up();

#endif /* HX711_h */
