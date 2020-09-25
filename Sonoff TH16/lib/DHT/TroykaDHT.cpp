/****************************************************************************/
//  Function:       Cpp file for TroykaDHT
//  Hardware:       DHT11, DHT21, DHT22
//  Arduino IDE:    Arduino-1.8.2
//  Author:         Igor Dementiev
//  Date:           Feb 22,2018
//  Version:        v1.0
//  by www.amperka.ru
/****************************************************************************/

#include "TroykaDHT.h"

DHT::DHT(uint8_t pin, uint8_t type) {
    _pin = pin;
    _type = type;
}

void DHT::begin() {
}

int8_t DHT::read() {

    uint8_t data[5];

    uint8_t dataBit;

    uint8_t checkSum;

    for (int i = 0; i < 5; i++)
        data[i] = 0;

    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
    //Original value is 18
    delayMicroseconds(500);

    pinMode(_pin, INPUT_PULLUP);

    if (pulseInLength(_pin, HIGH, 40) == 0) {
        _state = DHT_ERROR_NO_REPLY;
        return _state;
    }

    if (pulseInLength(_pin, LOW, 80) == 0) {
        _state = DHT_ERROR_NO_REPLY;
        return _state;
    }

    if (pulseInLength(_pin, HIGH, 80) == 0) {
        _state = DHT_ERROR_NO_REPLY;
        return _state;
    }


    for (int i = 0; i < 40; i++) {
        pulseInLength(_pin, LOW, 50);
        dataBit = pulseInLength(_pin, HIGH, 100);
        if (dataBit) {
            data[i / 8] <<= 1;
            data[i / 8] += dataBit > 45 ? 1 : 0;
        } else { 
            _state = DHT_ERROR_TIMEOUT;
            return _state;
        }
    }


    checkSum = data[0] + data[1] + data[2] + data[3];

    if (data[4] != checkSum) {
        _state = DHT_ERROR_CHECKSUM;
        return _state;
    }


    switch (_type) {
        case DHT11:
            _humidity = data[0];
            _temperatureC = data[3] & 0x80 ? (data[2] + (1 - (data[3] & 0x7F) * 0.1)) * -1 : (data[2] + (data[3] & 0x7F) * 0.1);
            _temperatureF = (_temperatureC * 9.0 / 5.0) + 32.0;
            _temperatureK = _temperatureC + CELSIUS_TO_KELVIN;
            break;
        case DHT21:
            _humidity = ((data[0] << 8) + data[1]) * 0.1;
            _temperatureC = (((data[2] & 0x7F) << 8) + data[3]) * (data[2] & 0x80 ? -0.1 : 0.1);
            _temperatureF = (_temperatureC * 9.0 / 5.0) + 32.0;
            _temperatureK = _temperatureC + CELSIUS_TO_KELVIN;
            break;
        case DHT22:
            _humidity = ((data[0] << 8) + data[1]) * 0.1;
            _temperatureC = (((data[2] & 0x7F) << 8) + data[3]) * (data[2] & 0x80 ? -0.1 : 0.1);
            _temperatureF = (_temperatureC * 9.0 / 5.0) + 32.0;
            _temperatureK = _temperatureC + CELSIUS_TO_KELVIN;
            break;
    }

    _state = DHT_OK;
    return _state;
}

unsigned long DHT::pulseInLength(uint8_t pin, bool state, unsigned long timeout) {
    unsigned long startMicros = micros();

    while (digitalRead(pin) == state) {
        if (micros() - startMicros > timeout)
            return 0;
    }
    return micros() - startMicros;
}