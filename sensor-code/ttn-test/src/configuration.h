#pragma once

#include <Arduino.h>
#include <lmic.h>

// -----------------------------------------------------------------------------
// Configuration
// -----------------------------------------------------------------------------

#define SERIAL_BAUD             115200          // Serial debug baud rate
#define TX_INTERVAL             600              // Seconds between each transmit
#define LORAWAN_PORT            1               // Port the messages will be sent to
#define LORAWAN_SF              DR_SF10         // Spreading factor (recommended DR_SF7 for ttn network map purposes, DR_SF10 works for slow moving trackers)
#define DATA_LENGTH             16              // Size of TX buffer sent over LoRa

// -----------------------------------------------------------------------------
// LoRa SPI
// -----------------------------------------------------------------------------

#define SCK_GPIO        5
#define MISO_GPIO       19
#define MOSI_GPIO       27
#define NSS_GPIO        18
#define RESET_GPIO      14
#define DIO0_GPIO       26
#define DIO1_GPIO       33 // Note: not really used on this board
#define DIO2_GPIO       32 // Note: not really used on this board