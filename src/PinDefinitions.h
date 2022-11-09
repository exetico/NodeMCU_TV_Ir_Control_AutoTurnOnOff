#if defined(ESP8266)
#define FEEDBACK_LED_IS_ACTIVE_LOW // The LED on my board (D4) is active LOW
#define IR_RECEIVE_PIN          5 // D1  = GPIO 5
#define IR_SEND_PIN             4 // D2 = GPIO4 is internal LED
#define _IR_TIMING_TEST_PIN     13 // D7
#define APPLICATION_PIN          0 // D3

#define tone(...) void()      // tone() inhibits receive timer
#define noTone(a) void()
#define TONE_PIN                42 // Dummy for examples using it

#elif defined(ESP32)
#include <Arduino.h>

// tone() is included in ESP32 core since 2.0.2
#if !defined(ESP_ARDUINO_VERSION_VAL)
#define ESP_ARDUINO_VERSION_VAL(major, minor, patch) 12345678
#endif
#if ESP_ARDUINO_VERSION  <= ESP_ARDUINO_VERSION_VAL(2, 0, 2)
#define TONE_LEDC_CHANNEL        1  // Using channel 1 makes tone() independent of receiving timer -> No need to stop receiving timer.
void tone(uint8_t aPinNumber, unsigned int aFrequency){
    ledcAttachPin(aPinNumber, TONE_LEDC_CHANNEL);
    ledcWriteTone(TONE_LEDC_CHANNEL, aFrequency);
}
void tone(uint8_t aPinNumber, unsigned int aFrequency, unsigned long aDuration){
    ledcAttachPin(aPinNumber, TONE_LEDC_CHANNEL);
    ledcWriteTone(TONE_LEDC_CHANNEL, aFrequency);
    delay(aDuration);
    ledcWriteTone(TONE_LEDC_CHANNEL, 0);
}
void noTone(uint8_t aPinNumber){
    ledcWriteTone(TONE_LEDC_CHANNEL, 0);
}
#endif // ESP_ARDUINO_VERSION  <= ESP_ARDUINO_VERSION_VAL(2, 0, 2)

#define IR_RECEIVE_PIN          15  // D15
#define IR_SEND_PIN              4  // D4
#define TONE_PIN                27  // D27 25 & 26 are DAC0 and 1
#define APPLICATION_PIN         16  // RX2 pin
#else
#warning Board / CPU is not detected using pre-processor symbols -> using default values, which may not fit. Please extend PinDefinitionsAndMore.h.
// Default valued for unidentified boards
#define IR_RECEIVE_PIN      2
#define IR_SEND_PIN         3
#define TONE_PIN            4
#define APPLICATION_PIN     5
#define ALTERNATIVE_IR_FEEDBACK_LED_PIN 6 // E.g. used for examples which use LED_BUILDIN for example output.
#define _IR_TIMING_TEST_PIN 7
#endif // defined(ESP8266)

#if defined(ESP32)
#define SEND_PWM_BY_TIMER // We do not have pin restrictions for this CPU's, so lets use the hardware PWM for send carrier signal generation
#else
# if defined(SEND_PWM_BY_TIMER)
#undef IR_SEND_PIN // SendPin is determined by timer! This avoids warning in IRTimer.hpp
#  endif
#endif

#if !defined (FLASHEND)
#define FLASHEND 0xFFFF // Dummy value for platforms where FLASHEND is not defined
#endif

/*
 * Helper macro for getting a macro definition as string
 */
#if !defined(STR_HELPER)
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#endif
