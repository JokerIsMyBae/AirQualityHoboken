#include <Arduino.h>
#include <Wire.h>

#include <SensirionI2CSen5x.h>
#include <lmic.h>
#include <hal/hal.h>

#include "configuration.h"
#include "credentials.h"

void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

static uint8_t txBuffer[DATA_LENGTH];
static osjob_t sendjob;

RTC_DATA_ATTR bool senReady = true;
RTC_DATA_ATTR lmic_t RTC_LMIC;

// Bool to enable ESP to go into deep sleep
// After deep sleep is over do_send() is executed.
// Following do_send(), os_runloop_once() is called continuously until TXCOMPLETE 
// After TXCOMPLETE, put ESP to sleep.
bool gotosleep = false;

// Pin mapping
const lmic_pinmap lmic_pins = {
    .nss = NSS_GPIO,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = RESET_GPIO,
    .dio = {DIO0_GPIO, DIO1_GPIO, DIO2_GPIO},
};

SensirionI2CSen5x sen55;
float f_pm1p0, f_pm2p5, f_pm4p0, f_pm10p0, f_hum, f_temp, f_voc, f_nox;

void saveLMICToRTC() {
  Serial.println("Saving LMIC to RTC memory.");
  RTC_LMIC = LMIC;

  unsigned long now = millis();

  for (int i = 0; i < MAX_BANDS; i++) {
    ostime_t correctedAvail = RTC_LMIC.bands[i].avail - ((now / 1000.0 + 10) * OSTICKS_PER_SEC);
    if (correctedAvail < 0) {
      correctedAvail = 0;
    }
    RTC_LMIC.bands[i].avail = correctedAvail;
  }

  RTC_LMIC.globalDutyAvail = RTC_LMIC.globalDutyAvail - ((now / 1000.0 + 10) * OSTICKS_PER_SEC);
  if (RTC_LMIC.globalDutyAvail < 0) {
    RTC_LMIC.globalDutyAvail = 0;
  }
}

void loadLMICFromRTC() {
  LMIC = RTC_LMIC;
}

void doDeepSleep(byte sleepSeconds) {
  esp_sleep_enable_timer_wakeup(sleepSeconds * 1000000); // in µs
  esp_deep_sleep_start();
}

void do_send(osjob_t* j){
  sen55.readMeasuredValues(
    f_pm1p0, f_pm2p5, f_pm4p0, f_pm10p0, f_hum, f_temp, f_voc, f_nox
  );

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

  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    Serial.println(F("OP_TXRXPEND, not sending"));
  } else {
    // Prepare upstream data transmission at the next possible time.
    LMIC_setTxData2(1, txBuffer, DATA_LENGTH, 0);
    LMIC_setDrTxpow(LORAWAN_SF, 14);
    Serial.println(F("Packet queued"));
  }
  // Next TX is scheduled after TX_COMPLETE event.
}

void onEvent (ev_t ev) {
  switch(ev) {
    case EV_SCAN_TIMEOUT:
      Serial.println(F("EV_SCAN_TIMEOUT"));
      break;
    case EV_BEACON_FOUND:
      Serial.println(F("EV_BEACON_FOUND"));
      break;
    case EV_BEACON_MISSED:
      Serial.println(F("EV_BEACON_MISSED"));
      break;
    case EV_BEACON_TRACKED:
      Serial.println(F("EV_BEACON_TRACKED"));
      break;
    case EV_JOINING:
      Serial.println(F("EV_JOINING"));
      break;
    case EV_JOINED:
      Serial.println(F("EV_JOINED"));
      {
        u4_t netid = 0;
        devaddr_t devaddr = 0;
        u1_t nwkKey[16];
        u1_t artKey[16];
        LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
        Serial.print("netid: ");
        Serial.println(netid, DEC);
        Serial.print("devaddr: ");
        Serial.println(devaddr, HEX);
        Serial.print("AppSKey: ");
        for (size_t i=0; i<sizeof(artKey); ++i) {
          Serial.print(artKey[i], HEX);
          Serial.print(" ");
        }
        Serial.println("");
        Serial.print("NwkSKey: ");
        for (size_t i=0; i<sizeof(nwkKey); ++i) {
          Serial.print(nwkKey[i], HEX);
          Serial.print(" ");
        }
        Serial.println();
      }
      // Disable link check validation (automatically enabled
      // during join, but because slow data rates change max TX
      // size, we don't use it.
      LMIC_setLinkCheckMode(0);
      break;
    case EV_JOIN_FAILED:
      Serial.println(F("EV_JOIN_FAILED"));
      break;
    case EV_REJOIN_FAILED:
      Serial.println(F("EV_REJOIN_FAILED"));
      break;
    case EV_TXCOMPLETE:
      Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
      if (LMIC.txrxFlags & TXRX_ACK)
        Serial.println(F("Received ack"));
      if (LMIC.dataLen) {
        Serial.print(F("Received "));
        Serial.print(LMIC.dataLen);
        Serial.println(F(" bytes of payload"));
      }
      gotosleep = true;
      break;
    case EV_LOST_TSYNC:
      Serial.println(F("EV_LOST_TSYNC"));
      break;
    case EV_RESET:
      Serial.println(F("EV_RESET"));
      break;
    case EV_RXCOMPLETE:
      Serial.println(F("EV_RXCOMPLETE"));
      break;
    case EV_LINK_DEAD:
      Serial.println(F("EV_LINK_DEAD"));
      break;
    case EV_LINK_ALIVE:
      Serial.println(F("EV_LINK_ALIVE"));
      break;
    case EV_TXSTART:
      Serial.println(F("EV_TXSTART"));
      break;
    case EV_TXCANCELED:
      Serial.println(F("EV_TXCANCELED"));
      break;
    case EV_RXSTART:
      /* do not print anything -- it wrecks timing */
      break;
    case EV_JOIN_TXCOMPLETE:
      Serial.println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
      break;

    default:
      Serial.print(F("Unknown event: "));
      Serial.println((unsigned) ev);
      break;
  }
}

void setup() {

  Serial.begin(SERIAL_BAUD);
  while (!Serial);
  Serial.println(F("Starting"));

  Wire.begin();
  sen55.begin(Wire);

  // LMIC init
  os_init();
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();

  if (RTC_LMIC.seqnoUp) {
    loadLMICFromRTC();
  }

  // Start job (sending automatically starts OTAA too)
  if (senReady) {
    do_send(&sendjob);
  }
}

void loop() {
  if (gotosleep) {
    sen55.stopMeasurement();
    senReady = false;

    saveLMICToRTC();
    doDeepSleep(SLEEP_S);
  } else if (!senReady) {
    Serial.println("Sleep 2 min for sensor delay.");
    senReady = true;
    doDeepSleep(120);
  } else { // if gotosleep == false and senready == true, check for queued packet
    os_runloop_once();
  }
}
