#include <Arduino.h>
#include <Wire.h>

#include <lmic.h>
#include <SensirionI2CSgp41.h>

#include <configuration.h>
#include <sleep.h>
#include <ttn.h>

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
  byte data[4];

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

