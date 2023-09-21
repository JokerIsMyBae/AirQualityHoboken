# SEN55

idle: 
typisch: 2.6mA - max: 3mA

Measurement-Mode (first 60 seconds):
typisch: 70mA - max: 100mA


# SGP41 VOC&nox sensor

idle:
typisch: 34µA-105µA (3,3V)

conditioning mode:
typisch: 4,2mA - max: 4,6mA (3,3V)

regular:
typisch: 3,0mA - max: 3,4mA (3,3V)



# ESP32-WROOM-32-N8 

- deepsleep -> 10 µA
- power off -> 1 µA
- Hibernation -> 5 µA
- Light-sleep -> 0.8 mA
- Modem-sleep:
  - CPU speed 240MHz:
    - dual core -> 30 mA ~ 68 mA
  - CPU speed 160MHz:
    - single core -> 27 mA ~ 44 mA
    - dual core -> 27 mA ~ 34 mA
  - CPU speed 80Hz:
    - single core -> 20 mA ~ 25 mA
    - dual core -> 20 mA ~ 31 mA

# SC1276-7-8

- Sleep mode -> typical 0.2µA, max 1µA
- Idle mode -> 1.5 1µA
- recieving mode -> 12mA
- transmit mode 
  - +20 dBm -> 120mA
  - +17 dBm -> 87mA
  - +13 dBm -> 29mA
  - +7 dBm -> 20mA



# totaal:
idle: 105µA + 3mA
Active: 3,4mA + 100mA


