#include <Arduino.h>
#include <Wire.h>

#include <SensirionI2CSen5x.h>
#include <lmic.h>
#include <hal/hal.h>

#include "configuration.h"
#include "credentials.h"


SensirionI2CSen5x sen55;

void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

static uint8_t txBuffer[DATA_LENGTH];
static osjob_t sendjob;

RTC_DATA_ATTR lmic_t RTC_LMIC;
RTC_DATA_ATTR bool senReady = true, gotosleep = false;
// gotosleep = bool to enable ESP to go into deep sleep
// After deep sleep is over do_send() is executed.
// Following do_send(), os_runloop_once() is called continuously until TXCOMPLETE 
// After TXCOMPLETE, put ESP to sleep.

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

void LoraWANPrintLMICOpmode(void)
{
    Serial.print(F("LMIC.opmode: "));
    if (LMIC.opmode & OP_NONE) { Serial.print(F("OP_NONE ")); }
    if (LMIC.opmode & OP_SCAN) { Serial.print(F("OP_SCAN ")); }
    if (LMIC.opmode & OP_TRACK) { Serial.print(F("OP_TRACK ")); }
    if (LMIC.opmode & OP_JOINING) { Serial.print(F("OP_JOINING ")); }
    if (LMIC.opmode & OP_TXDATA) { Serial.print(F("OP_TXDATA ")); }
    if (LMIC.opmode & OP_POLL) { Serial.print(F("OP_POLL ")); }
    if (LMIC.opmode & OP_REJOIN) { Serial.print(F("OP_REJOIN ")); }
    if (LMIC.opmode & OP_SHUTDOWN) { Serial.print(F("OP_SHUTDOWN ")); }
    if (LMIC.opmode & OP_TXRXPEND) { Serial.print(F("OP_TXRXPEND ")); }
    if (LMIC.opmode & OP_RNDTX) { Serial.print(F("OP_RNDTX ")); }
    if (LMIC.opmode & OP_PINGINI) { Serial.print(F("OP_PINGINI ")); }
    if (LMIC.opmode & OP_PINGABLE) { Serial.print(F("OP_PINGABLE ")); }
    if (LMIC.opmode & OP_NEXTCHNL) { Serial.print(F("OP_NEXTCHNL ")); }
    if (LMIC.opmode & OP_LINKDEAD) { Serial.print(F("OP_LINKDEAD ")); }
    if (LMIC.opmode & OP_LINKDEAD) { Serial.print(F("OP_LINKDEAD ")); }
    if (LMIC.opmode & OP_TESTMODE) { Serial.print(F("OP_TESTMODE ")); }
    if (LMIC.opmode & OP_UNJOIN) { Serial.print(F("OP_UNJOIN ")); }
    Serial.println("");
}

void LoraWANDebug(lmic_t lmic_to_check) {
    Serial.println("");
    Serial.println("");
    
    LoraWANPrintLMICOpmode();

    Serial.print("LMIC.seqnoUp = ");
    Serial.println(lmic_to_check.seqnoUp); 

    Serial.print("LMIC.globalDutyRate = ");
    Serial.print(lmic_to_check.globalDutyRate);
    Serial.print(" osTicks, ");
    Serial.print(osticks2ms(lmic_to_check.globalDutyRate)/1000);
    Serial.println(" sec");

    Serial.print("LMIC.globalDutyAvail = ");
    Serial.print(lmic_to_check.globalDutyAvail);
    Serial.print(" osTicks, ");
    Serial.print(osticks2ms(lmic_to_check.globalDutyAvail)/1000);
    Serial.println(" sec");

    Serial.print("LMICbandplan_nextTx = ");
    Serial.print(LMICbandplan_nextTx(os_getTime()));
    Serial.print(" osTicks, ");
    Serial.print(osticks2ms(LMICbandplan_nextTx(os_getTime()))/1000);
    Serial.println(" sec");

    Serial.print("os_getTime = ");
    Serial.print(os_getTime());
    Serial.print(" osTicks, ");
    Serial.print(osticks2ms(os_getTime()) / 1000);
    Serial.println(" sec");

    Serial.print("LMIC.txend = ");
    Serial.print(lmic_to_check.txend);
    Serial.print(" osticks, ");
    Serial.print(osticks2ms(lmic_to_check.txend) / 1000);
    Serial.println(" sec");

    Serial.print("LMIC.txChnl = ");
    Serial.println(lmic_to_check.txChnl);

    Serial.println("Band \tavail \t\tavail_sec\tlastchnl \ttxcap");
    for (u1_t bi = 0; bi < MAX_BANDS; bi++)
    {
        Serial.print(bi);
        Serial.print("\t");
        Serial.print(lmic_to_check.bands[bi].avail);
        Serial.print("\t\t");
        Serial.print(osticks2ms(lmic_to_check.bands[bi].avail)/1000);
        Serial.print("\t\t");
        Serial.print(lmic_to_check.bands[bi].lastchnl);
        Serial.print("\t\t");
        Serial.println(lmic_to_check.bands[bi].txcap);
        
    }
    Serial.println("");
    Serial.println("");
}

void doDeepSleep(uint16_t sleepSeconds) {
  Serial.flush();
  esp_sleep_enable_timer_wakeup(sleepSeconds * 1000000); // in µs
  esp_deep_sleep_start();
}

void do_send(osjob_t* j){
  float f_pm1p0, f_pm2p5, f_pm4p0, f_pm10p0, f_hum, f_temp, f_voc, f_nox;

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
    int error_code = LMIC_setTxData2(LORAWAN_PORT, txBuffer, DATA_LENGTH, 0);
    LMIC_setDrTxpow(LORAWAN_SF, 14);

    Serial.print("Error = ");
    Serial.println(error_code);
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

  Serial.println(F("Executing void setup()"));

  Wire.begin();
  sen55.begin(Wire);

  // LMIC init
  os_init();
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();
  // Disable adaptive data rate/adaptive spread factor
  LMIC_setAdrMode(false);

  if (RTC_LMIC.seqnoUp && senReady) {
    loadLMICFromRTC();
  }

  LoraWANDebug(LMIC);

  // Start job (sending automatically starts OTAA too)
  if (senReady) {
    do_send(&sendjob);
  }
}

void loop() {
  if (gotosleep) {
    Serial.println("Stopping sen55");
    sen55.stopMeasurement();
    senReady = false;
    gotosleep = false;
    saveLMICToRTC();
    doDeepSleep(SLEEP_S);
  } else if (!senReady) {
    Serial.println("Starting sen55");
    sen55.startMeasurement();
    senReady = true;
    doDeepSleep(SENSOR_SETUP_S);
  } else {
    os_runloop_once();
  }
}
