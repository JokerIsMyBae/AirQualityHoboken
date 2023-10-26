#include <Arduino.h>
#include <Wire.h>

#include <SensirionI2CSen5x.h>

#include <configuration.h>
#include <sleep.h>
#include <ttn.h>

#define DATA_LENGTH 32

SensirionI2CSen5x sen55;

bool packetSent, packetQueued;

static uint8_t txBuffer[DATA_LENGTH];

// deep sleep support
RTC_DATA_ATTR int bootCount = 0;

/**
   If we have a valid position send it to the server.
   @return true if we decided to send.
*/
bool trySend() {
  packetSent = false;
  ttn_send(txBuffer, DATA_LENGTH, LORAWAN_PORT, false);
  return true;
}

void doDeepSleep(uint64_t secToWake)
{
  Serial.printf("Entering deep sleep for %llu seconds\n", secToWake);

  LMIC_shutdown(); // cleanly shutdown the radio

  // sleep_millis(msecToWake); // also an option  
  sleep_seconds(secToWake); 
}

void callback(uint8_t message) {
  bool ttn_joined = false;
  if (EV_JOINED == message) {
    ttn_joined = true;
  }

  // We only want to say 'packetSent' for our packets (not packets needed for joining)
  if (EV_TXCOMPLETE == message && packetQueued) {
    packetQueued = false;
    packetSent = true;
  }

  if (EV_RESPONSE == message) {

    size_t len = ttn_response_len();
    uint8_t data[len];
    ttn_response(data, len);

    char buffer[6];
    for (uint8_t i = 0; i < len; i++) {
      snprintf(buffer, sizeof(buffer), "%02X", data[i]);
    }
  }
}

uint16_t measurementLoop(
    float& massConcentrationPm1p0, float& massConcentrationPm2p5, float& massConcentrationPm4p0, 
    float& massConcentrationPm10p0, float& ambientHumidity, float& ambientTemperature, float& vocIndex, 
    float& noxIndex) {
    uint16_t error = 0x0000;

    Serial.println("Starting measurement");
    // Wake up sensor and start measurements
    error = sen55.startMeasurement();
    if (error)
        return error;
    
    // SLEEP MCU FOR 2.5 TO 3 MINUTES FOR RELIABLE MEASUREMENTS
    delay(180000);

    // Read the measurements
    error = sen55.readMeasuredValues(
        massConcentrationPm1p0, massConcentrationPm2p5, massConcentrationPm4p0,
        massConcentrationPm10p0, ambientHumidity, ambientTemperature, vocIndex,
        noxIndex
    );
    if (error)
        return error;

    Serial.println("Read the measurements");
    
    // Return sensor to sleep mode 
    error = sen55.stopMeasurement();

    Serial.println("Back to sleep");
    return error;
}


void setup() {

  Serial.begin(SERIAL_BAUD);

  while (!Serial) {
    delay(100);
  }

  Wire.begin();
  sen55.begin(Wire);

  bootCount++;

  // TTN setup
  if (!ttn_setup()) {
    if (REQUIRE_RADIO) {
      delay(MESSAGE_TO_SLEEP_DELAY);
      sleep_forever();
    }
  }
  else {
    ttn_register(callback);
    ttn_join();
    ttn_adr(LORAWAN_ADR);
  }
}


void loop() {
  uint16_t error;
  char errorMsg[256];
  float massConcentrationPm1p0, massConcentrationPm2p5, massConcentrationPm4p0, 
  massConcentrationPm10p0, ambientHumidity, ambientTemperature, vocIndex, noxIndex;
  byte data[4];

  error = measurementLoop(
    massConcentrationPm1p0, massConcentrationPm2p5, massConcentrationPm4p0, 
    massConcentrationPm10p0, ambientHumidity, ambientTemperature, vocIndex, noxIndex
    );
  if (error) {
    Serial.print("Error trying to execute measurements: ");
    errorToString(error, errorMsg, 256);
    Serial.println(errorMsg);
  }
  else {
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

  uint32_t pm1p0, pm2p5, pm4p0, pm10p0, hum, temp, voc, nox;

  pm1p0 = massConcentrationPm1p0 * 100;
  pm2p5 = massConcentrationPm2p5 * 100;
  pm4p0 = massConcentrationPm4p0 * 100;
  pm10p0 = massConcentrationPm10p0 * 100;
  hum = ambientHumidity * 100;
  temp = (ambientTemperature +10) * 100;
  voc = vocIndex * 100;
  nox = noxIndex * 100;

  uint32_t data[8] = { pm1p0, pm2p5, pm4p0, pm10p0, hum, temp, voc, nox };

  for (byte i = 0; i < 8; i++) {
    txBuffer[0+i*4] = (data[i] >> 24) & 0xFF;
    txBuffer[1+i*4] = (data[i] >> 16) & 0xFF;
    txBuffer[2+i*4] = (data[i] >> 8) & 0xFF;
    txBuffer[3+i*4] = (data[i]) & 0xFF;
  }

  ttn_loop();

  if (packetSent) {
    packetSent = false;
  }

  // Send every SEND_INTERVAL millis
  static uint32_t last = 0;
  static bool first = true;
  if (0 == last || millis() - last > SEND_INTERVAL) {

    if (trySend()) {
      last = millis();
      first = false;
      Serial.println("TRANSMITTED");

    } else {
      if (first) {
        first = false;
      }
      // let the OS put the main CPU in low power mode for 100ms (or until another interrupt comes in)
      // i.e. don't just keep spinning in loop as fast as we can.
      delay(100);
    }

  }
}

