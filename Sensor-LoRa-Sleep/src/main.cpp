#include <Arduino.h>
#include <Wire.h>

#include <SensirionI2CSen5x.h>

#include <configuration.h>
#include <sleep.h>
#include <ttn.h>

SensirionI2CSen5x sen55;

bool packetSent, packetQueued;
static uint8_t txBuffer[DATA_LENGTH];
float f_pm1p0, f_pm2p5, f_pm4p0, f_pm10p0, f_hum, f_temp, f_voc, f_nox;

/**
   If we have a valid position send it to the server.
   @return true if we decided to send.
*/
bool try_send() {
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

uint16_t read_measurements(SensirionI2CSen5x &sen55, float &pm1p0, float &pm2p5, float &pm4p0, 
  float &pm10p0, float &hum, float &temp, float &voc, float &nox) {

  uint16_t error = 0x0000;

  error = sen55.readMeasuredValues(pm1p0, pm2p5, pm4p0, pm10p0, hum, temp, voc, nox);
  if (error) 
    return error;
  else
    return sen55.stopMeasurement();
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  while (!Serial) {
    delay(100);
  }

  Wire.begin();
  sen55.begin(Wire);

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
  uint16_t error = 0x0000;
  char errorMsg[256];

  Serial.print("Starting measurements...");
  error = sen55.startMeasurement();

  if (!error) {
    // SLEEP MCU FOR 2.5 TO 3 MINUTES FOR RELIABLE MEASUREMENTS
    doDeepSleep(120);

    Serial.print("Awake again.");

    error = read_measurements(
      sen55, f_pm1p0, f_pm2p5, f_pm4p0, f_pm10p0, f_hum, f_temp, f_voc, f_nox
      );
  }
  
  if (error) {
    Serial.print("Error trying to execute measurements: ");
    errorToString(error, errorMsg, 256);
    Serial.println(errorMsg);
  }
  else {
    Serial.print("MassConcentrationPm1p0:");
    Serial.print(f_pm1p0);
    Serial.print("\t");
    Serial.print("MassConcentrationPm2p5:");
    Serial.print(f_pm2p5);
    Serial.print("\t");
    Serial.print("MassConcentrationPm4p0:");
    Serial.print(f_pm4p0);
    Serial.print("\t");
    Serial.print("MassConcentrationPm10p0:");
    Serial.print(f_pm10p0);
    Serial.print("\t");
    Serial.print("AmbientHumidity:");
    if (isnan(f_hum)) 
      Serial.print("n/a");
    else 
      Serial.print(f_hum);
    Serial.print("\t");
    Serial.print("AmbientTemperature:");
    if (isnan(f_temp)) 
      Serial.print("n/a");
    else 
      Serial.print(f_temp);
    Serial.print("\t");
    Serial.print("VocIndex:");
    if (isnan(f_voc))
      Serial.print("n/a");
    else
      Serial.print(f_voc);
    Serial.print("\t");
    Serial.print("NoxIndex:");
    if (isnan(f_nox))
      Serial.println("n/a");
    else
      Serial.println(f_nox);

    uint32_t u_pm1p0 = f_pm1p0 * 100;
    uint32_t u_pm2p5 = f_pm2p5 * 100;
    uint32_t u_pm4p0 = f_pm4p0 * 100;
    uint32_t u_pm10p0 = f_pm10p0 * 100;
    uint32_t u_hum = f_hum * 100;
    uint32_t u_temp = (f_temp + 10) * 100;
    uint32_t u_voc = f_voc * 100;
    uint32_t u_nox = f_nox * 100;

    uint32_t data[8] = { u_pm1p0, u_pm2p5, u_pm4p0, u_pm10p0, u_hum, u_temp, u_voc, u_nox };

    for (byte i = 0; i < 8; i++) {
      txBuffer[0+i*2] = (data[i] >> 8) & 0xFF;
      txBuffer[1+i*2] = (data[i]) & 0xFF;
    }

    for (byte i = 0; i < 16; i++) {
      Serial.print(txBuffer[i]);
      Serial.print("\t");
    }

    ttn_loop();

    if (packetSent) 
      packetSent = false;

    // Send every SEND_INTERVAL millis
    static uint32_t last = 0;
    static bool first = true;
    if (last == 0 || millis() - last > SEND_INTERVAL) {
      if (try_send()) {
        last = millis();
        first = false;
        Serial.println("TRANSMITTED");
      } else {
        if (first) 
          first = false;
        // let the OS put the main CPU in low power mode for 100ms (or until another interrupt comes in)
        // i.e. don't just keep spinning in loop as fast as we can.
        delay(100);
      }
    }
  }
}
