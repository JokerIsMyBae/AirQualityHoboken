/*
Sleep module
*/
#include "sleep.h"
#include <esp_sleep.h>

void sleep_millis(uint64_t ms) {
    esp_sleep_enable_timer_wakeup(ms * 1000);
    esp_deep_sleep_start();
}

void sleep_seconds(uint64_t seconds) {
    esp_sleep_enable_timer_wakeup(seconds * 1000000);
    esp_deep_sleep_start();
}

void sleep_forever() {
    esp_deep_sleep_start();
}
