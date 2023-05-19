# IoT Temperature Controller

Humidity and temperature controller is part of smart home infrastructure.

## Description

<img height="320" width="750" src="/_readmi-res/elements.png">

The main features of the IoT controller:
1. Based on ESP8266/ESP32. It connects via Wifi to a router for Internet access;
2. One AM2320 Humidity and temperature sensor can be connected to the controller via I2C;
3. One or more DS18B20 sensors can be connected to the controller via 1-Wire;
4. Sensors data is periodically sent to the Firebase RealTime DataBase;
5. The IoT receives from Firebase RTDB a sensors polling time period (in seconds);
6. To initialization Wifi network parameters (SSID and Passkey), the device switches to Wifi-AP mode and starts Web server. The user, through a mobile application, connects to the WiFi controller and transfers the Wifi network parameters to it to access the Internet;
7. The Ping-Pong scheme allows the user to know if the device is online.
8. Additionally, a passive Buzzer (like KY-006) is installed on the device. Since the AM2320 sensor is connected via a very long cable, sometimes the connection between the controller and the sensor is lost. The buzzer signals this problem.

[To manage the IoT devices of this smart home system, there is a special mobile cross-platform application](https://github.com/0kmMobi/iots_manager).

