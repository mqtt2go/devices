# MQTT2GO Devices
Alternative firmware for ESP8266 based devices with MQTT2GO protocol compatibility.

## Supported devices

* [Shelly Plug S](./Shelly%20Plug%20S)
* [Shelly 2.5](./Shelly%202.5)
* [Shelly Plug S](./Sonoff%B1)
* [Shelly Plug S](./Sonoff%20POW%202)
* [Shelly Plug S](./Sonoff%20S26)
* [Shelly Plug S](./Sonoff%20TH16)

## Prerequisites
This alternative firmware has been developed in [Visual Studio Code](https://code.visualstudio.com/) utilizing [Platform IO](https://platformio.org/) plugin. For full functionality also the [Arduino IDE](https://www.arduino.cc/en/main/software) is required to be installed by the time of the firmware compilation.
All required dependencies should be installed automatically via the Platform IO plugin. If you are planning to install the dependencies (external libraries) manually, all the required header files are listed in __//External libraries__ comment.

The basic functionality is ensured by libraries:
* PubSubClient [link](https://github.com/knolleary/pubsubclient),
* ArduinoJson [link](https://github.com/bblanchon/ArduinoJson).

These libraries are used in each firmware mutation. Individual devices then require additional libraries needed for their functionality.
