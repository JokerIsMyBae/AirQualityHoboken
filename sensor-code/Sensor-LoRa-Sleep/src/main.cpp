#include <Arduino.h>
#include <Wire.h>

#include <SensirionI2CSen5x.h>
#include <lmic.h>
#include <hal/hal.h>

#include "configuration.h"
#include "credentials.h"

#ifdef LORA_DEBUG
#include "lora_debug.h"
#endif

SensirionI2CSen5x sen55;

// These functions are used by OTAA
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

static uint8_t txBuffer[DATA_LENGTH];
static osjob_t sendjob;

// Data to be put in RTC memory; doesn't get deleted in deep sleep
RTC_DATA_ATTR lmic_t RTC_LMIC;
RTC_DATA_ATTR bool senReady = true, gotosleep = false;

// Pin mapping
const lmic_pinmap lmic_pins = {
    .nss = NSS_GPIO,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = RESET_GPIO,
    .dio = {DIO0_GPIO, DIO1_GPIO, DIO2_GPIO},
};


void saveLMICToRTC() {
  Serial.println("Saving LMIC to RTC memory");
  RTC_LMIC = LMIC;

  // Compensate availability timer values for each band; else it keeps going up and tx_start interval gets bigger
  unsigned long now = millis();

  for (int i = 0; i < MAX_BANDS; i++) {
    ostime_t correctedAvail = RTC_LMIC.bands[i].avail - ((now / 1000.0 + TX_INTERVAL) * OSTICKS_PER_SEC);
    if (correctedAvail < 0) {
      correctedAvail = 0;
    }
    RTC_LMIC.bands[i].avail = correctedAvail;
  }

  RTC_LMIC.globalDutyAvail = RTC_LMIC.globalDutyAvail - ((now / 1000.0 + TX_INTERVAL) * OSTICKS_PER_SEC);
  if (RTC_LMIC.globalDutyAvail < 0) {
    RTC_LMIC.globalDutyAvail = 0;
  }
}

void loadLMICFromRTC() {
  Serial.println("Loading LMIC from RTC memory");
  LMIC = RTC_LMIC;
}

void doDeepSleep(uint16_t sleepSeconds) {
  Serial.flush();
  esp_sleep_enable_timer_wakeup(sleepSeconds * 1000000); // in µs
  esp_deep_sleep_start();
}

void do_send(osjob_t* j){
  Serial.println("Sending");
  float f_pm1p0, f_pm2p5, f_pm4p0, f_pm10p0, f_hum, f_temp, f_voc, f_nox;

  sen55.readMeasuredValues(
    f_pm1p0, f_pm2p5, f_pm4p0, f_pm10p0, f_hum, f_temp, f_voc, f_nox
  );

  // Convert floats to uints for smaller data size
  // Payload formatter in TTN reverts conversion
  uint32_t u_pm1p0 = f_pm1p0 * 100;
  uint32_t u_pm2p5 = f_pm2p5 * 100;
  uint32_t u_pm4p0 = f_pm4p0 * 100;
  uint32_t u_pm10p0 = f_pm10p0 * 100;
  uint32_t u_hum = f_hum * 100;
  uint32_t u_temp = (f_temp + 10) * 100;
  uint32_t u_voc = f_voc * 100;
  uint32_t u_nox = f_nox * 100;

  uint32_t data[8] = { u_pm1p0, u_pm2p5, u_pm4p0, u_pm10p0, u_hum, u_temp, u_voc, u_nox };

  // Put data in txBuffer
  for (byte i = 0; i < 8; i++) {
    txBuffer[0+i*2] = (data[i] >> 8) & 0xFF;
    txBuffer[1+i*2] = (data[i]) & 0xFF;
  }

  // Prepare upstream data transmission at the next possible time.
  lmic_tx_error_t error = LMIC_setTxData2_strict(LORAWAN_PORT, txBuffer, DATA_LENGTH, 0);
  Serial.print("Error: ");
  Serial.println(error);
  if (error) {
    Serial.println("Error detected, not sending; going to sleep");
    LMIC_reset();
    doDeepSleep(60);
    // Put MCU back to sleep; restarts tx loop, queue new packet afterwards 
  } else {
    // Set spread factor to SF in configuration
    LMIC_setDrTxpow(LORAWAN_SF, 14);
    Serial.println(F("Packet queued"));
    // Next TX is scheduled after TX_COMPLETE event.
  }
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
        }
        Serial.println("");
        Serial.print("NwkSKey: ");
        for (size_t i=0; i<sizeof(nwkKey); ++i) {
          Serial.print(nwkKey[i], HEX);
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
      // Go to sleep on completion of TX
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

  // Init communication with sensor
  Wire.begin();
  sen55.begin(Wire);

  // LMIC init
  os_init();
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();
  // Disable adaptive data rate/adaptive spread factor
  LMIC_setAdrMode(false);

  // Only load LMIC config when planning to send (aka sensor is ready to be read)
  if (RTC_LMIC.seqnoUp && senReady) {
    loadLMICFromRTC();
  }

  #ifdef LORA_DEBUG
    LoraWANDebug(LMIC);
  #endif

  // Start job (sending automatically starts OTAA too)
  if (senReady) {
    do_send(&sendjob);
  }
}

void loop() {
  if (gotosleep) {
    // Turn off sensor, save LMIC config and put esp in deep sleep
    Serial.println("Stopping sen55");
    sen55.stopMeasurement();
    senReady = false;
    gotosleep = false;
    bit_t lmic_status = LMIC_queryTxReady();
    while (!lmic_status) {
      Serial.println("LMIC not ready");
      delay(500);
      lmic_status = LMIC_queryTxReady();
    }
    saveLMICToRTC();
    doDeepSleep(SLEEP_S);
  
  } else if (!senReady) {
    // Start sensor and go back to deep sleep waiting for sensor setup time
    Serial.println("Starting sen55");
    sen55.startMeasurement();
    senReady = true;
    doDeepSleep(SENSOR_SETUP_S);
  } else {
    // Check if any jobs are queued; if so, execute job
    os_runloop_once();
  }
}
