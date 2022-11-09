/*
 * SimpleSender.cpp
 *
 *  Demonstrates sending IR codes in standard format with address and command
 *  An extended example for sending can be found as SendDemo.
 *
 *  Copyright (C) 2020-2022  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of Arduino-IRremote https://github.com/Arduino-IRremote/Arduino-IRremote.
 *
 *  MIT License
 */
#include <Arduino.h>

//#define SEND_PWM_BY_TIMER         // Disable carrier PWM generation in software and use (restricted) hardware PWM.
//#define USE_NO_SEND_PWM           // Use no carrier PWM, just simulate an active low receiver signal. Overrides SEND_PWM_BY_TIMER definition

#include "PinDefinitionsAndMore.h" // Define macros for input and output pin etc.
#include <IRremote.hpp>

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.begin(115200);

    // Just to know which program is running on my Arduino
    Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_IRREMOTE));

    /*
     * The IR library setup. That's all!
     */
//    IrSender.begin(); // Start with IR_SEND_PIN as send pin and if NO_LED_FEEDBACK_CODE is NOT defined, enable feedback LED at default feedback LED pin
    IrSender.begin(DISABLE_LED_FEEDBACK); // Start with IR_SEND_PIN as send pin and disable feedback LED at default feedback LED pin

    Serial.print(F("Ready to send IR signals at pin "));
    Serial.println(IR_SEND_PIN);

    //   LED Build In NodeMCU
    pinMode(D4, OUTPUT);
}

/*
 * Set up the data to be sent.
 * For most protocols, the data is build up with a constant 8 (or 16 byte) address
 * and a variable 8 bit command.
 * There are exceptions like Sony and Denon, which have 5 bit address.
 */
uint16_t sAddress = 0x707;
uint8_t sCommandTurnOff = 0x98; // Turn Off
uint8_t sCommandTurnOn = 0x99; // Turn On
uint8_t sRepeats = 10;

void loop() {
    /*
     * Print current send values
     */
    Serial.println();
    Serial.print(F("Send now: address=0x"));
    Serial.print(sAddress, HEX);
    Serial.print(F(" repeats="));
    Serial.print(sRepeats);
    Serial.println();


    Serial.println();
    Serial.print(F(" sCommandTurnOn=0x"));
    Serial.print(sCommandTurnOn, HEX);
    Serial.println();
    IrSender.sendSamsung(sAddress, sCommandTurnOn, sRepeats);

    // delay must be greater than 5 ms (RECORD_GAP_MICROS), otherwise the receiver sees it as one long signal
    digitalWrite(D4, LOW);
    delay(300);
    digitalWrite(D4, HIGH);
    delay(300);
    digitalWrite(D4, LOW);
    delay(300);
    digitalWrite(D4, HIGH);
    delay(300);
    digitalWrite(D4, LOW);
    delay(300);
    digitalWrite(D4, HIGH);
    delay(300);
    digitalWrite(D4, LOW);
    delay(300);
    digitalWrite(D4, HIGH);
    delay(300);
    digitalWrite(D4, LOW);
    delay(300);
    digitalWrite(D4, HIGH);
    delay(300);
    digitalWrite(D4, LOW);
    delay(300);
    digitalWrite(D4, HIGH);
    delay(300);
    digitalWrite(D4, LOW);
    delay(300);
    digitalWrite(D4, HIGH);
    delay(300);
    digitalWrite(D4, LOW);
    delay(300);
    digitalWrite(D4, HIGH);
    delay(300);
    digitalWrite(D4, LOW);
    delay(300);
    digitalWrite(D4, HIGH);
    delay(300);
    digitalWrite(D4, LOW);
    delay(300);
    digitalWrite(D4, HIGH);
    delay(300);
    digitalWrite(D4, LOW);
    delay(300);
    digitalWrite(D4, HIGH);
    delay(300);

    Serial.println();
    Serial.print(F(" sCommandTurnOff=0x"));
    Serial.print(sCommandTurnOff, HEX);
    Serial.println();
    IrSender.sendSamsung(sAddress, sCommandTurnOff, sRepeats);

    digitalWrite(D4, LOW);
    delay(600);
    digitalWrite(D4, HIGH);
    delay(600);
    digitalWrite(D4, LOW);
    delay(600);
    digitalWrite(D4, HIGH);
    delay(600);
    digitalWrite(D4, LOW);
    delay(600);
    digitalWrite(D4, HIGH);
    delay(600);
    digitalWrite(D4, LOW);
    delay(600);
    digitalWrite(D4, HIGH);
    delay(600);
    digitalWrite(D4, LOW);
    delay(600);
    digitalWrite(D4, HIGH);
    delay(600);

}
