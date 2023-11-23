#include <stdint.h>
#include <lmic.h>

void _ttn_callback(uint8_t message);

void forceTxSingleChannelDr();

void gen_lora_deveui(uint8_t *pdeveui);

static void printHex2(unsigned v);

void onEvent(ev_t event);

void ttn_register(void (*callback)(uint8_t message));

size_t ttn_response_len();

void ttn_response(uint8_t * buffer, size_t len);

static void initCount();

bool ttn_setup();

void ttn_join();

void ttn_sf(unsigned char sf);

void ttn_adr(bool enabled);

uint32_t ttn_get_count();

static void ttn_set_cnt();

void ttn_send(uint8_t * data, uint8_t data_size, uint8_t port, bool confirmed);

void ttn_loop();
