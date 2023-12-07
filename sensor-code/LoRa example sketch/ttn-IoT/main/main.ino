#include "configuration.h"
#include "rom/rtc.h"
#include <Wire.h>

// Sensor code sgp41
#include <SensirionI2CSgp41.h>

#define DEFAULT_RH 0x8000
#define DEFAULT_T  0x6666

SensirionI2CSgp41 sgp41;

bool pmu_irq = false;

bool packetSent, packetQueued;

#if defined(PAYLOAD_USE_FULL)
// includes number of satellites and accuracy
static uint8_t txBuffer[10];
#elif defined(PAYLOAD_USE_CAYENNE)
// CAYENNE DF
static uint8_t txBuffer[11] = {0x03, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#endif


// deep sleep support
RTC_DATA_ATTR int bootCount = 0;
esp_sleep_source_t wakeCause; // the reason we booted this time

// -----------------------------------------------------------------------------
// Application
// -----------------------------------------------------------------------------

void buildPacket(uint8_t txBuffer[]); // needed for platformio


bool trySend() {
  packetSent = false;
  ttn_send(txBuffer, sizeof(txBuffer), LORAWAN_PORT, false);
  return true;
}


void doDeepSleep(uint64_t msecToWake)
{
  Serial.printf("Entering deep sleep for %llu seconds\n", msecToWake / 1000);

  // not using wifi yet, but once we are this is needed to shutoff the radio hw
  // esp_wifi_stop();

  LMIC_shutdown(); // cleanly shutdown the radio

  // FIXME - use an external 10k pulldown so we can leave the RTC peripherals powered off
  // until then we need the following lines
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

  // Only GPIOs which are have RTC functionality can be used in this bit map: 0,2,4,12-15,25-27,32-39.
  uint64_t gpioMask = (1ULL << BUTTON_PIN);

  // FIXME change polarity so we can wake on ANY_HIGH instead - that would allow us to use all three buttons (instead of just the first)
  gpio_pullup_en((gpio_num_t) BUTTON_PIN);

  esp_sleep_enable_ext1_wakeup(gpioMask, ESP_EXT1_WAKEUP_ALL_LOW);

  esp_sleep_enable_timer_wakeup(msecToWake * 1000ULL); // call expects usecs
  esp_deep_sleep_start();                              // TBD mA sleep current (battery)
}


void sleep() {
#if SLEEP_BETWEEN_MESSAGES
  // Wait for MESSAGE_TO_SLEEP_DELAY millis to sleep
  delay(MESSAGE_TO_SLEEP_DELAY);
}

// Set the user button to wake the board
sleep_interrupt(BUTTON_PIN, LOW);

// We sleep for the interval between messages minus the current millis
// this way we distribute the messages evenly every SEND_INTERVAL millis
uint32_t sleep_for = (millis() < SEND_INTERVAL) ? SEND_INTERVAL - millis() : SEND_INTERVAL;
doDeepSleep(sleep_for);

#endif
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


void scanI2Cdevice(void)
{
  byte err, addr;
  int nDevices = 0;
  for (addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    err = Wire.endTransmission();
    if (err == 0) {
      Serial.print("I2C device found at address 0x");
      if (addr < 16)
        Serial.print("0");
      Serial.print(addr, HEX);
      Serial.println(" !");
      nDevices++;
    }
    if (nDevices == 0)
      Serial.println("No I2C devices found\n");
    else
      Serial.println("done\n");
  }
}

// Perform power on init that we do on each wake from deep sleep
void initDeepSleep() {
  bootCount++;
  wakeCause = esp_sleep_get_wakeup_cause();

  Serial.printf("booted, wake cause %d (boot count %d)\n", wakeCause, bootCount);
}


void setup() {
  Serial.begin(115200);

  // sensor sgp 41
  while (!Serial) {
    delay(100);
  }

  Wire.begin();
  sgp41.begin(Wire);


  // Debug
#ifdef DEBUG_PORT
  DEBUG_PORT.begin(SERIAL_BAUD);
#endif

  initDeepSleep();

  Wire.begin(I2C_SDA, I2C_SCL);
  scanI2Cdevice();


  // Buttons & LED
  pinMode(BUTTON_PIN, INPUT_PULLUP);

#ifdef LED_PIN
  pinMode(LED_PIN, OUTPUT);
#endif

  // Hello
  DEBUG_MSG(APP_NAME " " APP_VERSION "\n");

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

  // sensor sgp41
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
    sleep();
  }


  // if user presses button for more than 3 secs, discard our network prefs and reboot (FIXME, use a debounce lib instead of this boilerplate)
  static bool wasPressed = false;
  static uint32_t minPressMs; // what tick should we call this press long enough
  if (!digitalRead(BUTTON_PIN)) {
    if (!wasPressed) { // just started a new press
      Serial.println("pressing");
      wasPressed = true;
      minPressMs = millis() + 3000;
    }
  } else if (wasPressed) {
    // we just did a release
    wasPressed = false;
    if (millis() > minPressMs) {
      // held long enough
#ifndef PREFS_DISCARD
#endif
#ifdef PREFS_DISCARD
      ttn_erase_prefs();
      delay(5000);
      ESP.restart();
#endif
    }
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

      // No GPS lock yet, let the OS put the main CPU in low power mode for 100ms (or until another interrupt comes in)
      // i.e. don't just keep spinning in loop as fast as we can.
      delay(100);
    }
  }
}
