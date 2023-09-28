#include <Arduino.h>
#include <SensirionI2CSen5x.h>
#include <Wire.h>

SensirionI2CSen5x sen55;

uint16_t error;
char errorMsg[256];
bool dataReady = false;

float massConcentrationPm1p0, massConcentrationPm2p5, massConcentrationPm4p0, 
massConcentrationPm10p0, ambientHumidity, ambientTemperature, vocIndex, noxIndex;

void setup() {
  
  Serial.begin(115200);
    while (!Serial) {
        delay(100);
    }

  Wire.begin();
  sen55.begin(Wire);

}



void loop() {
  
  error = sen55.startMeasurement();
  if (error) {
    Serial.print("Error trying to execute startMeasurement(): ");
    errorToString(error, errorMsg, 256);
    Serial.println(errorMsg);
  }
  
  do {
    error = sen55.readDataReady(dataReady);
    if (error) {
      Serial.print("Error trying to read dataReady flag: ");
      errorToString(error, errorMsg, 256);
      Serial.println(errorMsg);
    }
    delay(1);
  } while (!dataReady);

  error = sen55.readMeasuredValues(
    massConcentrationPm1p0, massConcentrationPm2p5, massConcentrationPm4p0,
    massConcentrationPm10p0, ambientHumidity, ambientTemperature, vocIndex,
    noxIndex
    );
  if (error) {
    Serial.print("Error trying to execute readMeasuredValues(): ");
    errorToString(error, errorMsg, 256);
    Serial.println(errorMsg);
  } else {
        Serial.print("MassConcentrationPm1p0:");
        Serial.print(massConcentrationPm1p0);
        Serial.print("\t");
        Serial.print("MassConcentrationPm2p5:");
        Serial.print(massConcentrationPm2p5);
        Serial.print("\t");
        Serial.print("MassConcentrationPm4p0:");
        Serial.print(massConcentrationPm4p0);
        Serial.print("\t");
        Serial.print("MassConcentrationPm10p0:");
        Serial.print(massConcentrationPm10p0);
        Serial.print("\t");
        Serial.print("AmbientHumidity:");
        if (isnan(ambientHumidity)) {
            Serial.print("n/a");
        } else {
            Serial.print(ambientHumidity);
        }
        Serial.print("\t");
        Serial.print("AmbientTemperature:");
        if (isnan(ambientTemperature)) {
            Serial.print("n/a");
        } else {
            Serial.print(ambientTemperature);
        }
        Serial.print("\t");
        Serial.print("VocIndex:");
        if (isnan(vocIndex)) {
            Serial.print("n/a");
        } else {
            Serial.print(vocIndex);
        }
        Serial.print("\t");
        Serial.print("NoxIndex:");
        if (isnan(noxIndex)) {
            Serial.println("n/a");
        } else {
            Serial.println(noxIndex);
        }
    }

  error = sen55.stopMeasurement();
  if (error) {
    Serial.print("Error returning sensor to idle: ");
    errorToString(error, errorMsg, 256);
    Serial.println(errorMsg);
  }

}
