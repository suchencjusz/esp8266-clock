# ESP 8266 - Dual screen clock

A simple clock created on esp8266 from the modules I have
- ESP8266
- Two ssd1306 screens
- Si7021 I2C

Moreover, the clock connects to the API
- [Airly](https://airly.org/en/) (air quality) 
- [Github API](https://docs.github.com/en/rest) (info about last repo commit)

Libraries
- thingpulse/ESP8266 and ESP32 OLED driver for SSD1306 displays
- arduino-libraries/NTPClient
- arkhipenko/TaskScheduler
- adafruit/Adafruit Si7021 Library
- bblanchon/ArduinoJson
- adafruit/Adafruit BusIO
