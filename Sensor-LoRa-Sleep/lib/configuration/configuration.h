//https://github.com/matthijskooijman/arduino-lmic
#pragma once

#include <Arduino.h>
#include <lmic.h>
void ttn_register(void (*callback)(uint8_t message));

// -----------------------------------------------------------------------------
// Version
// -----------------------------------------------------------------------------

#define APP_NAME                "TTN MAP-TRACK"
#define APP_VERSION             "1.2.1"

// -----------------------------------------------------------------------------
// Configuration
// -----------------------------------------------------------------------------

// Select which T-Beam board is being used. Only uncomment one.
//#define T_BEAM_V07  // AKA Rev0 (first board released)
 #define T_BEAM_V10  // AKA Rev1 (second board released)

// Select the payload format. Change on TTN as well. Only uncomment one.
#define PAYLOAD_USE_FULL
// #define PAYLOAD_USE_CAYENNE

// If using a single-channel gateway, uncomment this next option and set to your gateway's channel
//#define SINGLE_CHANNEL_GATEWAY  0

//Uncomment to enable discarding network settings by long pressing second button
//#define PREFS_DISCARD

// If you are having difficulty sending messages to TTN after the first successful send,
// uncomment the next option and experiment with values (~ 1 - 5)
//#define CLOCK_ERROR             5

#define DEBUG_PORT              Serial          // Serial debug port
#define SERIAL_BAUD             115200          // Serial debug baud rate
#define SLEEP_BETWEEN_MESSAGES  false           // Do sleep between messages
#define SEND_INTERVAL           (20 * 1000)     // Sleep for these many millis
#define MESSAGE_TO_SLEEP_DELAY  5000            // Time after message before going to sleep
#define LORAWAN_PORT            10              // Port the messages will be sent to
#define LORAWAN_CONFIRMED_EVERY 0               // Send confirmed message every these many messages (0 means never)
#define LORAWAN_SF              DR_SF10         // Spreading factor (recommended DR_SF7 for ttn network map purposes, DR_SF10 works for slow moving trackers)
#define LORAWAN_ADR             0               // Enable ADR
#define REQUIRE_RADIO           true            // If true, we will fail to start if the radio is not found
#define DATA_LENGTH             16              // Size of TX buffer sent over LoRa

// If not defined, we will wait for lock forever
#define GPS_WAIT_FOR_LOCK       (60 * 1000)     // Wait after every boot for GPS lock (may need longer than 5s because we turned the gps off during deep sleep)

// -----------------------------------------------------------------------------
// DEBUG
// -----------------------------------------------------------------------------

#ifdef DEBUG_PORT
#define DEBUG_MSG(...) DEBUG_PORT.printf( __VA_ARGS__ )
#else
#define DEBUG_MSG(...)
#endif

// -----------------------------------------------------------------------------
// Custom messages
// -----------------------------------------------------------------------------

#define EV_QUEUED       100
#define EV_PENDING      101
#define EV_ACK          102
#define EV_RESPONSE     103

// -----------------------------------------------------------------------------
// General
// -----------------------------------------------------------------------------

#define I2C_SDA         21
#define I2C_SCL         22

#if defined(T_BEAM_V07)
#define LED_PIN         14
#define BUTTON_PIN      39
#elif defined(T_BEAM_V10)
#define BUTTON_PIN      38
#endif

// -----------------------------------------------------------------------------
// GPS
// -----------------------------------------------------------------------------

#define GPS_SERIAL_NUM  1
#define GPS_BAUDRATE    9600
#define USE_GPS         0

#if defined(T_BEAM_V07)
#define GPS_RX_PIN      12
#define GPS_TX_PIN      15
#elif defined(T_BEAM_V10)
#define GPS_RX_PIN      34
#define GPS_TX_PIN      12
#endif

// -----------------------------------------------------------------------------
// LoRa SPI
// -----------------------------------------------------------------------------

#define SCK_GPIO        5
#define MISO_GPIO       19
#define MOSI_GPIO       27
#define NSS_GPIO        18
#if defined(T_BEAM_V10)
#define RESET_GPIO      14
#else
#define RESET_GPIO      23
#endif
#define DIO0_GPIO       26
#define DIO1_GPIO       33 // Note: not really used on this board
#define DIO2_GPIO       32 // Note: not really used on this board

// -----------------------------------------------------------------------------
// AXP192 (Rev1-specific options)
// -----------------------------------------------------------------------------

// #define AXP192_SLAVE_ADDRESS  0x34 // Now defined in axp20x.h
#define GPS_POWER_CTRL_CH     3
#define LORA_POWER_CTRL_CH    2
#define PMU_IRQ               35
