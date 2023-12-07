#include "lora_debug.h"

void LoraWANPrintLMICOpmode(void) {
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