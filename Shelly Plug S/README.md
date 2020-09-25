# Shelly Plug S

## Preparation/Flashing procedure

1. Unscrew the screw situated on the backside of the Shelly Plug S, see <a href="#fig1">Fig. 1</a>.

<p align="center" >
	<img src="Shelly_plug_back.svg" alt="Shelly Plug S - back." height="300">
</p>
<p align="center" >
	<a name="fig1"></a><em><strong>Fig. 1:</strong>Shelly Plug S - back view.</em>
</p>

2. Take off the power socket lid, see <a href="#fig2">Fig. 2</a>.

<p align="center" >
	<img src="Shelly_plug.svg" alt="Shelly Plug S - front." height="300">
</p>
<p align="center" >
	<a name="fig2"></a><em><strong>Fig. 2:</strong>Shelly Plug S - front view.</em>
</p>

3. Connect the USB <-> UART converter to the socket PCB, according to <a href="#fig3">Fig. 3</a>. The GPIO 0 pin has to be grounded while applying the power to enter programming mode.

<p align="center" >
	<img src="Shelly_plug_open.svg" alt="Shelly Plug S - PCB." height="500">
</p>
<p align="center" >
	<a name="fig3"></a><em><strong>Fig. 3:</strong>Shelly Plug S - PCB.</em>
</p>

4. In this phase, the device is ready to be flashed with the new firmware. After the Platform IO: Upload button press, see <a href="#fig4">Fig. 4</a>, the source code will be compiled and uploaded to the device.

<p align="center" >
	<img src="platformio.svg" alt="VS Code Upload." height="48">
</p>
<p align="center" >
	<a name="fig4"></a><em><strong>Fig. 4:</strong>Platform IO - Upload.</em>
</p>

5. After reboot, the device starts the connection procedure according to the MQTT2GO standard, see the [link](https://mqtt2go.github.io/).

## Required External Dependencies

* PubSubClient [link](https://github.com/knolleary/pubsubclient),
* ArduinoJson [link](https://github.com/bblanchon/ArduinoJson),
* HLW8012 [link](https://github.com/xoseperez/hlw8012).


## Configurable Parameters

* _CLIENT_ID_ – represents the Device ID of MQTT2GO standard,
* _ACTIVATION_CODE_ – represents the Activation Code of MQTT2GO standard,
* _fingerprint_ – hashed fingerprint of the initial CA certificate,
* _REQ_SSID_ – SSID of the guest Wi-Fi utilized in adding procedure [link](https://mqtt2go.github.io/add-wifi.html).

