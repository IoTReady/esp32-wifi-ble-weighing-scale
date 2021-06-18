# ESP32 Weighing Scale

This is a weighing scale designed to share data over Wi-Fi using esp32 WROOM32 module.
For the weighing scale we are using a load cell and measuring the output using a HX711 24-bit ADC designed for weigh scales and industrial control applications to interface directly with a bridge sensor.

The final design and gerber files are available [here](https://github.com/esp32-wifi-ble-weighing-scale/esp32-wifi-ble-weighing-scale/blob/master/design/Single_scale_v3_kicad/single_scale_v3_gerber.zip).

# Circuit Design
## Power Supply
We are using a 3.7V Li-Ion battery for powering this scale. The current rating can be changed depending on the use case. We also have a battery charging circuit on the board, so you don't have to remove the battery for charging it.

![charging circuit](https://github.com/IoTReady/esp32-wifi-ble-weighing-scale/blob/master/design/Single_scale_v3_kicad/docs/charging_circuit.jpg "Battery Charging Circuit")

We use a TP4056 charging IC, as it is easily available and gives a reasonable 1000 mA output current, with 4.2V output voltage for charging the battery. For preventing the battery from over-charging and over-discharging, we use the DW01A protection IC. It is connected with a FS8205A switch IC, which will cut off the battery from the circuit if any fault is detected.

Finally, the 3.7V battery ouput is fed to two voltage regulators to get +5V and +3.3V output voltages as shown below:

![voltage regs](https://github.com/IoTReady/esp32-wifi-ble-weighing-scale/blob/master/design/Single_scale_v3_kicad/docs/voltage_regulators.jpg "Voltage Regulators")

## ADC Circuit
We are using HX711 24-bit ADC IC for converting the analog data from the Load cell. We selected HX711 as it is designed specifically for weighing scales, to interface with brige sensors directly. Load cells are basically bridge sensors.
As shown below the final output is in serial data output form. This is basically a UART output, and there are few digital pin controls.

![adc](https://github.com/IoTReady/esp32-wifi-ble-weighing-scale/blob/master/design/Single_scale_v3_kicad/docs/hx711_adc.jpg "HX711 ADC")

## ESP32
The brains of our weighing scale, the esp32 wroom32 module is a BLE plus WiFi SoC which collects weight data from the ADC circuit and outputs the final processed data via a digital display and WiFi to the selected servers for maintaing inventories or doing any data analysis.

![esp32](https://github.com/IoTReady/esp32-wifi-ble-weighing-scale/blob/master/design/Single_scale_v3_kicad/docs/esp32_circuit.jpg "esp32")


# PCB
This is what the final PCB looks like:

![pcb_front](https://github.com/IoTReady/esp32-wifi-ble-weighing-scale/blob/master/design/Single_scale_v3_kicad/docs/Front_side_3D_PCB.png "pcb_front")

![pcb_back](https://github.com/IoTReady/esp32-wifi-ble-weighing-scale/blob/master/design/Single_scale_v3_kicad/docs/Back_side_3D_PCB.png "pcb_back")

# Firmware

## Features
- 24 bit HX711 as ADC
- Any 4 wire load cell
- BLE communication
- Tare button
- Battery level monitor
- MAX7219 driven 7-segment display
- OTA (to be hosted from local network)
- Console setting of tare value and sensor calibration

### Weight Calculation
The HX711 is a great match for weighing use-cases. The 24 bit ADC is designed specifically for weight measurements and is directly interfaceable with load sensors and weigh bridges. There is no programming needed for the internal registers. All controls to the HX711 are through the pins. The HX711 component handles everything necessary for conversion to calibration. For more details, find the HX711 datasheet [here](https://cdn.sparkfun.com/datasheets/Sensors/ForceFlex/hx711_english.pdf)

### Factors for weight calculation/conversion
There are two major factors to consider in order to get an accurate calculation:
1. Tare Value (OFFSET): In weighment applications it is important to be able to reset the output value to zero if and when needed. In the firmware however, it is particularly important to understand the ADC output at zero. Any value higher or lower than this ADC value would result in positive or negative weight hereafter. Almost all weighing scales will provide a button for the users to return the current weight to zero. This is used to round off any extra weight that needs not be accounted, for example the container. In this application, the tare button has been assigned to GPIO19 on the ESP32.
  The auto-tare feature in the application tares the system at startup, such that the weighing scale always starts with a zero value. This eliminates human error. Make sure that there are no items placed on the platform and even the bottom of the scale is not in contact with any object at the time of starting up the machine until the display shows 0.
  
3. Calibration value (SCALE): The calibration value is the multiplication factor that needs to be multiplied to the ADC output value in order to get the real-time weight out of it in the units needed. This calibration value will be different for all individual load cells, hence it is important that all weighing machines go through a calibration procedure before they are shipped.

This application provides a simple method to set/calculate the above mentioned Tare and Calibration values:
- Start by placing the scale in a stable platform and environment. Please choose an area with minimum air currents and vibration levels.
- Connect your computer to the ESP32 controller's UART0 using an FTDI connector.
- You will need software to access the serial communication happening over the UART lines. For example, [PuTTY](https://www.putty.org/).
- While the program is flashed and running on the ESP32, press 'm' and hit Enter. You will be provided a menu using which you can access the tare and calibration features.
- For calibration, carry out the following steps:
  - You need to select a test value weight. This weight must be within the scale constraint and approximately 50% of the upper capacity. This should also be a known weight that you should possess.
  - Make sure that there are no items placed on the platform and even the bottom of the scale is not in contact with any object.
  - Make sure the console shows 0 weight. If not, press the tare button so the console shows 0.
  - Place the known selected weight on the platform of the weighing scale and view the weight on the console.
  - Follow the instructions to either change the calibration value or select the current selected calibration value if the weight shown on console is correct.
  - Repeat until the weight is correct.
  - Save

### BLE Characteristics:
| Service / Characteristics Name | UUID Handle                          | Properties | Description   |
|--------------------------------|--------------------------------------|------------|---------------|
| Device State Service           | 000000ff-0000-1000-8000-00805f9b34fb | Service    | -             |
| Weight Data                    | 0000ff00-0000-1000-8000-00805f9b34fb | Read       | Weight in kgs |


| Service / Characteristics Name  | UUID Handle                          | Properties  | Description                                                                                                       | Size            | Data Type        |
|---------------------------------|--------------------------------------|-------------|-------------------------------------------------------------------------------------------------------------------|-----------------|------------------|
| System Settings Service         | 000000ee-0000-1000-8000-00805f9b34fb | Service     | -                                                                                                                 | -               | -                |
| Device ID Characteristic        | 0000ee00-0000-1000-8000-00805f9b34fb | Read        | Read Device ID                                                                                                    | Length: 12      | String           |
| WiFi SSID  Characteristic       | 0000ee01-0000-1000-8000-00805f9b34fb | Read/Write  | WiFi SSID configured with the device                                                                              | Max Length: 32  | String           |
| WiFi Password Characteristic    | 0000ee02-0000-1000-8000-00805f9b34fb | Read/Write  | WiFi Password configured with the device                                                                          | Max Length: 32  | String           |
| OTA IP Characteristic           | 0000ee03-0000-1000-8000-00805f9b34fb | Read/Write  | Read/Write the IP of OTA host                                                                                     | Max Length: 100 | String           |
| OTA Port Characteristic         | 0000ee04-0000-1000-8000-00805f9b34fb | Read/Write  | Read/Write the Port of OTA host server                                                                            | Max Length: 100 | String           |
| OTA Filename Characteristic     | 0000ee05-0000-1000-8000-00805f9b34fb | Read/Write  | Read/Write the filename of OTA binary file                                                                        | Max Length: 100 | String           |
| Force OTA Characteristic        | 0000ee06-0000-1000-8000-00805f9b34fb | Write       | Initiate OTA procedure Value: ‘1’ [0x31]                                                                          | 1 byte          | Unsigned Integer |
| Tare Weight Characteristic      | 0000ee07-0000-1000-8000-00805f9b34fb | Write       | Cancel OTA procedure within 10 seconds of initiation Value: ‘1’ [0x31]                                            | 1 byte          | Unsigned Integer |
| Connect WiFi Characteristic     | 0000ee08-0000-1000-8000-00805f9b34fb | Write       | Connect to previously configured WiFi credentials. Value: ‘1’ [0x31]                                              | 1 byte          | Unsigned Integer |
| Software Version Characteristic | 0000ee09-0000-1000-8000-00805f9b34fb | Read        | Read the Software Version in the device                                                                           | -               | String           |
# Applications
This weighing scale is designed keeping in mind sharing of data over the internet. So, any application where we need to capture weight of things and maintain a record for any use, we can integrate this device in that system.

![license_info](https://github.com/IoTReady/esp32-wifi-ble-weighing-scale/blob/master/Screenshot_2021-05-27%20Facts%20generator.png "license_info")
