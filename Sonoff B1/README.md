# Shelly Plug S

## Preparation/Flashing procedure

1. Remove the lid of the Sonoff B1 smart bulb, see <a href="#fig1">Fig. 1</a>.

<p align="center" >
	<img src="B1.svg" alt="Sonoff B1 smart bulb" height="400">
</p>
<p align="center" >
	<a name="fig1"></a><em><strong>Fig. 1:</strong>Sonoff B1 smart bulb.</em>
</p>

2. Solder wires to the PCB  and connect the USB <-> UART converter to the socket PCB, according to <a href="#fig2">Fig. 2</a>. The GPIO 0 pin has to be grounded while applying the power to enter programming mode.

<p align="center" >
	<img src="B1_open.svg" alt="Sonoff B1 - PCB" height="500">
</p>
<p align="center" >
	<a name="fig2"></a><em><strong>Fig. 2:</strong>Sonoff B1 - PCB.</em>
</p>

3. In this phase, the device is ready to be flashed with the new firmware. After the Platform IO: Upload button press, see <a href="#fig3">Fig. 3</a>, the source code will be compiled and uploaded to the device.

<p align="center" >
	<img src="platformio.svg" alt="VS Code Upload." height="48">
</p>
<p align="center" >
	<a name="fig3"></a><em><strong>Fig. 3:</strong>Platform IO - Upload.</em>
</p>

4. After reboot, the device starts the connection procedure according to the MQTT2GO standard, see the [link](https://mqtt2go.github.io/).

## Required External Dependencies

* PubSubClient [link](https://github.com/knolleary/pubsubclient),
* ArduinoJson [link](https://github.com/bblanchon/ArduinoJson),
* MY92XX [link](https://github.com/xoseperez/my92xx),
* Arduino ColorConverter [link] (https://github.com/luisllamasbinaburo/Arduino-ColorConverter).


#include <ColorConverterLib.h>


## Configurable Parameters

* _CLIENT_ID_ – represents the Device ID of MQTT2GO standard,
* _ACTIVATION_CODE_ – represents the Activation Code of MQTT2GO standard,
* _fingerprint_ – hashed fingerprint of the initial CA certificate,
* _REQ_SSID_ – SSID of the guest Wi-Fi utilized in adding procedure [link](https://mqtt2go.github.io/add-wifi.html).

