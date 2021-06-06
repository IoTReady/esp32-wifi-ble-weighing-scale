# ESP32 Weighing Scale

This is a weighing scale designed to share data over Wi-Fi using esp32 WROOM32 module.
For the weighing scale we are using a load cell and measuring the output using a HX711 24-bit ADC designed for weigh scales and industrial control applications to interface directly with a bridge sensor.

The final design and gerber files are available [here](https://github.com/IoTReady/weighing_scale_firmware/blob/master/design/Single_scale_v3_kicad/single_scale_v3_gerber.zip).

# Circuit Design
## Power Supply
We are using a 3.7V Li-Ion battery for powering this scale. The current rating can be changed depending on the use case. We also have a battery charging circuit on the board, so you don't have to remove the battery for charging it.

![charging circuit](https://github.com/IoTReady/weighing_scale_firmware/blob/master/design/Single_scale_v3_kicad/docs/charging_circuit.jpg "Battery Charging Circuit")

We use a TP4056 charing IC, as it is easilt available and give a reasonable 1000 mA output current, with 4.2V output voltage for charing the battery. For preventing the battery from over-charging and over-discharging, we use DW01A protection IC. It is connected with a FS8205A switch IC, which will cut off the battery from the circuit if any fault is detected.

Finally, the 3.7V battery ouput is fed to two voltage regulators to get +5V and +3.3V output voltages as shown below:

![voltage regs](https://github.com/IoTReady/weighing_scale_firmware/blob/master/design/Single_scale_v3_kicad/docs/voltage_regulators.jpg "Voltage Regulators")

## ADC Circuit
We are using HX711 24-bit ADC IC for converting the analog data from the Load cell. We selected HX711 as it is designed specifically for weighing scales, to interface with brige sensors directly. And, load cells are basically brige sesnors.
As shown below the final output is in Serial data output form. This is basically a UART output, and there are few digital pin controls.

![adc](https://github.com/IoTReady/weighing_scale_firmware/blob/master/design/Single_scale_v3_kicad/docs/hx711_adc.jpg "HX711 ADC")

## ESP32
The brains of our weighing scale esp32 wroom32 module is a WiFi SoC which collects weight data from the ADC circuit and outputs the final processed data via a digital display and WiFi to the selected servers for maintaing inventories or doing any data analysis.

![esp32](https://github.com/IoTReady/weighing_scale_firmware/blob/master/design/Single_scale_v3_kicad/docs/esp32_circuit.jpg "esp32")


# PCB
This is what the final PCB looks like:

![pcb_front](https://github.com/IoTReady/weighing_scale_firmware/blob/master/design/Single_scale_v3_kicad/docs/Front_side_3D_PCB.png "pcb_front")

![pcb_back](https://github.com/IoTReady/weighing_scale_firmware/blob/master/design/Single_scale_v3_kicad/docs/Back_side_3D_PCB.png "pcb_back")

# Firmware

# Applications
This weighing scale is designed keeping in mind sharing of data over the internet. So, any application where we need to capture weight of things and maintain a record for any use, we can integrate this device in that system.
