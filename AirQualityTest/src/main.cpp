#include <Arduino.h>
#include <SensirionI2CSgp41.h>
#include <Wire.h>

#define DEFAULT_RH 0x8000
#define DEFAULT_T  0x6666

uint16_t measurementLoop(uint16_t& srawVoc, uint16_t& srawNox);

SensirionI2CSgp41 sgp41;

void setup() {

    Serial.begin(115200);
    while (!Serial) {
        delay(100);
    }

    // Initialise I²C comms and initialise the sensor
    Wire.begin();
    sgp41.begin(Wire);

}

void loop() {
    uint16_t error;
    char errorMsg[256];
    uint16_t srawVoc = 0, srawNox = 0;
    byte data[4];

    // Execute the commands needed for 1 measurement cycle 
    error = measurementLoop(srawVoc, srawNox);
    if (error) {
        Serial.print("Error trying to execute measurements: ");
        errorToString(error, errorMsg, 256);
        Serial.println(errorMsg);
    }
    else {
        Serial.print("SRAW_VOC: ");
        Serial.print(srawVoc);
        Serial.print("\t");
        Serial.print("SRAW_NOx: ");
        Serial.println(srawNox);
    }

    // Split measurements into bytes for sending over LoRaWAN
    data[0] = highByte(srawVoc);
    data[1] = lowByte(srawVoc);
    data[2] = highByte(srawNox);
    data[3] = lowByte(srawNox);

    for (byte i = 0; i < 4; i++) {
        Serial.println(data[i]);
    }
}

uint16_t measurementLoop(uint16_t& srawVoc, uint16_t& srawNox) {
    uint16_t error = 0x0000;

    // Run conditioning of sensor on wakeup, conditioning needs to take 10s
    unsigned long conditioningStartTime = millis();
    error = sgp41.executeConditioning(DEFAULT_RH, DEFAULT_T, srawVoc);
    if (error)
        return error;
    while ( (millis() - conditioningStartTime) <= 10000 ) { delay(1); };

    // Get measurement results, pass rel. hum. & temp. as parameters
    error = sgp41.measureRawSignals(DEFAULT_RH, DEFAULT_T, srawVoc, srawNox);
    if (error)
        return error;
    
    // Return sensor to sleep mode
    error = sgp41.turnHeaterOff();
    
    return error;
}
