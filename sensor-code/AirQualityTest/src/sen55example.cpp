// #include <Arduino.h>
// #include <SensirionI2CSen5x.h>
// #include <Wire.h>

// SensirionI2CSen5x sen55;

// uint16_t measurementLoop(
//     float& massConcentrationPm1p0, float& massConcentrationPm2p5, float& massConcentrationPm4p0, 
//     float& massConcentrationPm10p0, float& ambientHumidity, float& ambientTemperature, float& vocIndex, 
//     float& noxIndex); 

// void setup() {
  
//     Serial.begin(115200);
//     while (!Serial) {
//         delay(100);
//     }

//     // Initialise I²C comms and initialise the sensor
//     Wire.begin();
//     sen55.begin(Wire);

// }

// void loop() {
//     uint16_t error;
//     char errorMsg[256];
//     float massConcentrationPm1p0, massConcentrationPm2p5, massConcentrationPm4p0, 
//     massConcentrationPm10p0, ambientHumidity, ambientTemperature, vocIndex, noxIndex;

//     // Execute the commands needed for 1 measurement cycle 
//     error = measurementLoop(
//         massConcentrationPm1p0, massConcentrationPm2p5, massConcentrationPm4p0, 
//         massConcentrationPm10p0, ambientHumidity, ambientTemperature, vocIndex, 
//         noxIndex
//         );
//     if (error) {
//         Serial.print("Error trying to execute measurements: ");
//         errorToString(error, errorMsg, 256);
//         Serial.println(errorMsg);
//     }
//     else {
//         Serial.print("MassConcentrationPm1p0:");
//         Serial.print(massConcentrationPm1p0);
//         Serial.print("\t");
//         Serial.print("MassConcentrationPm2p5:");
//         Serial.print(massConcentrationPm2p5);
//         Serial.print("\t");
//         Serial.print("MassConcentrationPm4p0:");
//         Serial.print(massConcentrationPm4p0);
//         Serial.print("\t");
//         Serial.print("MassConcentrationPm10p0:");
//         Serial.print(massConcentrationPm10p0);
//         Serial.print("\t");
//         Serial.print("AmbientHumidity:");
//         if (isnan(ambientHumidity)) {
//             Serial.print("n/a");
//         } else {
//             Serial.print(ambientHumidity);
//         }
//         Serial.print("\t");
//         Serial.print("AmbientTemperature:");
//         if (isnan(ambientTemperature)) {
//             Serial.print("n/a");
//         } else {
//             Serial.print(ambientTemperature);
//         }
//         Serial.print("\t");
//         Serial.print("VocIndex:");
//         if (isnan(vocIndex)) {
//             Serial.print("n/a");
//         } else {
//             Serial.print(vocIndex);
//         }
//         Serial.print("\t");
//         Serial.print("NoxIndex:");
//         if (isnan(noxIndex)) {
//             Serial.println("n/a");
//         } else {
//             Serial.println(noxIndex);
//         }
//     }

//     delay(2000);
// }

// uint16_t measurementLoop(
//     float& massConcentrationPm1p0, float& massConcentrationPm2p5, float& massConcentrationPm4p0, 
//     float& massConcentrationPm10p0, float& ambientHumidity, float& ambientTemperature, float& vocIndex, 
//     float& noxIndex) {
//     uint16_t error = 0x0000;
//     bool dataReady = false;

//     // Wake up sensor and start measurements
//     error = sen55.startMeasurement();
//     if (error)
//         return error;
//     do {
//         // Wait until data is ready
//         error = sen55.readDataReady(dataReady);
//         if (error)
//             return error;
//         delay(1);
//     } while ( !dataReady );

//     // Read the measurements
//     error = sen55.readMeasuredValues(
//         massConcentrationPm1p0, massConcentrationPm2p5, massConcentrationPm4p0,
//         massConcentrationPm10p0, ambientHumidity, ambientTemperature, vocIndex,
//         noxIndex
//     );
//     if (error)
//         return error;
    
//     // Return sensor to sleep mode 
//     error = sen55.stopMeasurement();

//     return error;
// }
