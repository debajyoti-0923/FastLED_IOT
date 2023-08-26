This is the firmware for IOT based room decoration. Refer images.

Features-
1. Customizable colors theme.
2. Supports almost all types of Addressable LEDS.
3. WiFi and hotspot support.
4. Interactive webapp support (ambi.local).
5. APIs support.

USE-
1. First upload the data folder to the esp8266 using LittleFS (refer https://randomnerdtutorials.com/install-esp8266-nodemcu-littlefs-arduino/)
2. Upload the server_diy.ino code to esp8266 board.
3. Connect your laptop or mobile device to Ambi hotspot. Then open the following link (http://ambi.local/ or http://192.168.0.181/). Scroll down and provide your home wifi(2.4GHZ) username and password. Click save and restart.
4. Connect to your home wifi and then go to (http://ambi.local/ or http://192.168.0.181/) to access the webapp.

ESP8266 based Addresable LEDs controller with webserver functionality. ARGB Leds utilise FastLED library along with ESPwebserver to provide an interactive webapp to control the leds.

Credits-- Debjayoti Das# FastLED_IOT
