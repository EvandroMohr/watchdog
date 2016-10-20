
[![API Docs](http://watchdog.evandromohr.com.br)](http://watchdog.evandromohr.com.br)

# Watchdog
Arduino interface for antennas and device management. It allows you make manually reset, schedule a reset task or just adjust settings for auto management.


## Minimum Requirements

 * Arduino UNO (Atmega328)
 * Ethernet Shield w/ SD reader (W5100)
 * Relay module (For power supply management)

## Getting started

 * Clone the respository
 * ``` git clone https://emohr@bitbucket.org/emohr/watchdog.git ```
 * Upload the project to your arduino using an Arduino IDE (^1.6)

Go to your DHCP device and look for `WIZnetEFFEED` device to get the IP (or you can manually set it)
You can use the GRASS watchdog manager by setting this IP as the watchdog endpoint, or just make a request using your browser to interact with the device.

## API documentation

The API documentation can be found at:


```
http://watchdog.evandromohr.com.br/
```
