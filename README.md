# How to

## Libs

Most libs are added in "platformio.ini", but WifiManager is (as we speak) only released on 0.16.

Therefore WifiManger is added manually in `libs` from:
https://github.com/tzapu/WiFiManager

Cause we're using the `wm.getWebPortalActive()` function.

## Sources / Links

### Reference

### IR Control

https://github.com/crankyoldgit/IRremoteESP8266/tree/master/examples
https://www.cnx-software.com/2017/04/20/karls-home-automation-project-part-4-mqtt-bridge-updated-to-use-ys-irtm-ir-receiver-transmitter-with-nodemcu/

### Using the proper GPIO PINS

https://www.esp8266.com/viewtopic.php?p=90383#p90383

## Wirering with logic level converter

https://github.com/mcauser/micropython-ys-irtm
