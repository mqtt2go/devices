# MQTT2GO Devices
Alternative firmware for ESP8266 based devices with MQTT2GO protocol compatibility.

## Supported devices

* Shelly Plug S

## Prerequisites
This alternative firmware has been developed in Visual Studio Code utilizing Platform IO plugin. For full functionality also the Arduino IDE is required to be installed by the time of the firmware compilation.
All required dependencies should be installed automatically via the Platform IO plugin. If you are planning to install the dependencies (external libraries) manually, all the required header files are listed in //External libraries comment.

The basic functionality is ensured by libraries:
* PubSubClient [link](https://github.com/knolleary/pubsubclient),
* ArduinoJson [link](https://github.com/bblanchon/ArduinoJson).
These libraries are used in each firmware mutation. Individual devices then require additional libraries needed for their functionality.
