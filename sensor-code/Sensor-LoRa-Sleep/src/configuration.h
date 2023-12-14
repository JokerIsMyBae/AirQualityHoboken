#pragma once

#include <Arduino.h>
#include <lmic.h>

// -----------------------------------------------------------------------------
// Configuration
// -----------------------------------------------------------------------------

#define SERIAL_BAUD    115200                       // Serial debug baud rate
#define DATA_LENGTH    16                           // Size of TX buffer sent over LoRa
#define TX_INTERVAL    1.020                           // Seconds between each transmit
#define SENSOR_SETUP_S 120                           // Time needed for sensor to return valid measurements (at least 2 min)
#define SLEEP_S        TX_INTERVAL - SENSOR_SETUP_S // Seconds of deep sleep for ESP32 (= tx_interval - sensor setup time) 
#define LORAWAN_PORT   1                            // Port the messages will be sent to
#define LORAWAN_SF     EU868_DR_SF10                // Spreading factor (recommended DR_SF7 for ttn network map purposes, DR_SF10 works for slow moving trackers)

// -----------------------------------------------------------------------------
// Enable debugging
// -----------------------------------------------------------------------------

// #define LORA_DEBUG

// -----------------------------------------------------------------------------
// LoRa SPI
// -----------------------------------------------------------------------------

#define SCK_GPIO        18
#define MISO_GPIO       19
#define MOSI_GPIO       23
#define NSS_GPIO        5
#define RESET_GPIO      14
#define DIO0_GPIO       33
#define DIO1_GPIO       26  // Note: not really used on this board
#define DIO2_GPIO       32  // Note: not really used on this board
