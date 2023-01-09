/*
Joe Mulhern 2017
PWM colors for strips. Make with RGB LEDs
*/
#ifndef Color_h
#define Color_h

#include "Arduino.h"

class Color
{
public:
  Color(int redPIN, int greenPIN, int bluePIN);
  void whiteRed();
  void whiteRedDimmed();
  void whiteGreen();
  void whiteGreenDimmed();
  void whiteBlue();
  void whiteBlueDimmed();
  void whiteBoth();
  void off();
  void on();
  void blink();
  void blinkShort();

private:
  int _redPIN, _greenPIN, _bluePIN;
};

#endif
