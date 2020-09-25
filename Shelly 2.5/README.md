# Shelly 2.5

## Preparation/Flashing procedure

1. Connect the USB <-> UART converter to the pinheader on the back of the device, according to <a href="#fig1">Fig. 1</a>. The GPIO 0 pin has to be grounded while applying the power to enter programming mode.

<p align="center" >
	<img src="Shelly 25.svg" alt="Shelly 2.5." height="500">
</p>
<p align="center" >
	<a name="fig1"></a><em><strong>Fig. 1:</strong>Shelly 2.5.</em>
</p>

4. In this phase, the device is ready to be flashed with the new firmware. After the Platform IO: Upload button press, see <a href="#fig2">Fig. 2</a>, the source code will be compiled and uploaded to the device.

<p align="center" >
	<img src="platformio.svg" alt="VS Code Upload." height="48">
</p>
<p align="center" >
	<a name="fig2"></a><em><strong>Fig. 2:</strong>Platform IO - Upload.</em>
</p>

5. After reboot, the device starts the connection procedure according to the MQTT2GO standard, see the [link](https://mqtt2go.github.io/).

## Required External Dependencies

* PubSubClient [link](https://github.com/knolleary/pubsubclient),
* ArduinoJson [link](https://github.com/bblanchon/ArduinoJson),


## Configurable Parameters

* _CLIENT_ID_ – represents the Device ID of MQTT2GO standard,
* _ACTIVATION_CODE_ – represents the Activation Code of MQTT2GO standard,
* _fingerprint_ – hashed fingerprint of the initial CA certificate,
* _REQ_SSID_ – SSID of the guest Wi-Fi utilized in adding procedure [link](https://mqtt2go.github.io/add-wifi.html).

