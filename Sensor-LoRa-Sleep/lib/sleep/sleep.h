/*
Header file for ESP sleep interface
*/
#include <stdint.h>

void sleep_millis(uint64_t ms);

void sleep_seconds(uint32_t seconds);

void sleep_forever();