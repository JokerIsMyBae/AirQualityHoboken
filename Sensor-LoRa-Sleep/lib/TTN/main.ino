#include "rom/rtc.h"
#include <Wire.h>
#include <lmic.h>

#include <SensirionI2CSgp41.h>

#define LORAWAN_PORT 10
#define SERIAL_BAUD 115200
#define SEND_INTERVAL (20 * 1000)
#define DATA_LENGTH 10

#define DEFAULT_RH 0x8000
#define DEFAULT_T  0x6666

SensirionI2CSgp41 sgp41;

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

void doDeepSleep(uint64_t msecToWake)
{
  Serial.printf("Entering deep sleep for %llu seconds\n", msecToWake / 1000);

  LMIC_shutdown(); // cleanly shutdown the radio

  // FIXME - use an external 10k pulldown so we can leave the RTC peripherals powered off
  // until then we need the following lines
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

  esp_sleep_enable_timer_wakeup(msecToWake * 1000ULL); // call expects usecs
  esp_deep_sleep_start();                              // TBD mA sleep current (battery)
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


void setup() {

  Serial.begin(SERIAL_BAUD);

  while (!Serial) {
    delay(100);
  }

  Wire.begin();
  sgp41.begin(Wire);

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
    Serial.print("SRAW_VOC: ");
    Serial.print(srawVoc);
    Serial.print("\t");
    Serial.print("SRAW_NOx: ");
    Serial.println(srawNox);
  }

  error = sgp41.turnHeaterOff();
  if (error) {
    Serial.print("Error trying to execute turnHeaterOff(): ");
    errorToString(error, errorMsg, 256);
    Serial.println(errorMsg);
  }

  byte data[4];

  data[0] = highByte(srawVoc);
  data[1] = lowByte(srawVoc);
  data[2] = highByte(srawNox);
  data[3] = lowByte(srawNox);

  for (byte i = 0; i < 4; i++) {
    Serial.println(data[i]);
  }
  
 
 txBuffer[0] = highByte(srawVoc) & 0xFF;
 txBuffer[1] = lowByte(srawVoc) & 0xFF;
 txBuffer[2] = highByte(srawNox) & 0xFF;
 txBuffer[3] = lowByte(srawNox) & 0xFF;
  
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
