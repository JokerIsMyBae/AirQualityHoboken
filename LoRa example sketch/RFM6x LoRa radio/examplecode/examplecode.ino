//https://learn.circuit.rocks/long-range-lora-communication-for-devices-that-have-no-wifi-connection-available
//https://github.com/PaulStoffregen/RadioHead
#include <Arduino.h>
#include <RH_RF95.h>

// Change this to the GPIO connected to the RFM95 SS pin
#define NSS 5
// Change this to the GPIO connected to the RFM95 DIO0 pin
#define DIO0 26

// Define a LED port if not already defined
#ifndef LED_BUILTIN
// Change this to the GPIO connected to an LED
#define LED_BUILTIN 17
#endif

// Initialize the interface
RH_RF95 rf95(NSS, DIO0);

// The Ping message will be sent by the node
char pingMsg[] = "PING";
// The Pong message will be sent by the gateway
char pongMsg[] = "PONG";
// Tracks the time stamp of last packet received
long timeSinceLastPacket = 0;

void setup()
{
  // Initialize the LED port
  pinMode(LED_BUILTIN, OUTPUT);
  // Start serial communication
  Serial.begin(115200);

  Serial.println("=====================================");
  Serial.println("LoRa node (sender) test");
  Serial.println("=====================================");

  // Initialize LoRa
  if (!rf95.init())
  {
    Serial.println("LoRa init failed. Check your connections.");
    while (true)
      ;
  }

  // Set frequency (Philippines uses same frequency as Europe)
  rf95.setFrequency(868.3); // Select the frequency 868.3 MHz - Used in Europe

  // The default transmitter power is 13dBm, using PA_BOOST.
  // Set Transmitter power to 20dbm.
  rf95.setTxPower(20, false);
}

void loop()
{
  Serial.println("Sending a PING");
  rf95.send((uint8_t *)pingMsg, sizeof(pingMsg));
  rf95.waitPacketSent();

  // Now wait for a reply
  byte buf[RH_RF95_MAX_MESSAGE_LEN];
  byte len = sizeof(buf);

  if (rf95.waitAvailableTimeout(2000))
  {
    // Should be a reply message for us now
    if (rf95.recv(buf, &len))
    {
      Serial.print("Got reply: ");
      Serial.println((char *)buf);
      Serial.print(" RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);
    }
    else
    {
      Serial.println("Receive failed, is the Gateway running?");
    }
  }
  else
  {
    Serial.println("No reply, is the Gateway running?");
  }
  delay(500);
}
