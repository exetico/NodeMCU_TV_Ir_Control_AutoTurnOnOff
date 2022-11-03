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
https://www.instructables.com/Easiest-ESP8266-Learning-IR-Remote-Control-Via-WIF/

### Using the proper GPIO PINS

https://www.esp8266.com/viewtopic.php?p=90383#p90383

## Not used

https://www.cnx-software.com/2017/04/20/
karls-home-automation-project-part-4-mqtt-bridge-updated-to-use-ys-irtm-ir-receiver-transmitter-with-nodemcu/
https://github.com/mcauser/micropython-ys-irtm

## Notes about pin-out

https://www.electronicwings.com/nodemcu/nodemcu-gpio-with-arduino-ide

Quote:

```
Some of the GPIO pins are used while booting, so Pulling this pin HIGH or LOW can prevent NODEMCU from booting

    GPIO0: It oscillates and stabilizes HIGH after ~100ms. Boot Failure if pulled LOW
    GPIO1: LOW for ~50ms, then HIGH, Boot Failure if Pulled LOW.
    GPIO2: It oscillates and stabilize HIGH after ~100ms, Boot Failure if Pulled LOW.
    GPIO3: LOW for ~50ms, then HIGH.
    GPIO9: Pin is HIGH at Boot.
    GPIO10: Pin is HIGH at Boot.
    GPIO15: LOW, Boot failure if Pulled HIGH
    GPIO16: HIGH during Boot and Falls to ~1Volt.
```

## Other things

### ESP Basic

https://www.esp8266basic.com/editor-interface.html

Find inspiration here:
https://www.instructables.com/Easiest-ESP8266-Learning-IR-Remote-Control-Via-WIF/

IR codes:
http://www.righto.com/2009/08/multi-protocol-infrared-remote-library.html

Arduino IR Remote:
https://github.com/Arduino-IRremote/Arduino-IRremote
