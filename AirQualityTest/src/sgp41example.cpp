#include <Arduino.h>
#include <SensirionI2CSgp41.h>
#include <Wire.h>

#define DEFAULT_RH 0x8000
#define DEFAULT_T  0x6666

SensirionI2CSgp41 sgp41;

void setup() {

    Serial.begin(115200);
    while (!Serial) {
        delay(100);
    }

    Wire.begin();
    sgp41.begin(Wire);

}

void loop() {
    uint16_t error;
    char errorMsg[256];
    uint16_t srawVoc = 0, srawNox = 0;

    unsigned long conditioningStart = millis();
    error = sgp41.executeConditioning(DEFAULT_RH, DEFAULT_T, srawVoc);
    if (error) {
        Serial.print("Error trying to execute executeConditioning(): ");
        errorToString(error, errorMsg, 256);
        Serial.println(errorMsg);
    }
    while ( (millis() - conditioningStart) < 10000);

    error = sgp41.measureRawSignals(DEFAULT_RH, DEFAULT_T, srawVoc, srawNox);
    if (error) {
        Serial.print("Error trying to execute measureRawSignals(): ");
        errorToString(error, errorMsg, 256);
        Serial.println(errorMsg);
    } else {
        Serial.print("SRAW_VOC:");
        Serial.print(srawVoc);
        Serial.print("\t");
        Serial.print("SRAW_NOx:");
        Serial.println(srawNox);
    }

    error = sgp41.turnHeaterOff();
    if (error) {
        Serial.print("Error trying to execute turnHeaterOff(): ");
        errorToString(error, errorMsg, 256);
        Serial.println(errorMsg);
    }
}