/*
Joe Mulhern 2017
PWM colors for strips. Make with RGB LEDs
*/
#include "Color.h"

Color::Color(int redPIN, int greenPIN, int bluePIN)
{
  _redPIN = redPIN;
  _greenPIN = greenPIN;
  _bluePIN = bluePIN;
  pinMode(_redPIN, OUTPUT);
  pinMode(_greenPIN, OUTPUT);
  pinMode(_bluePIN, OUTPUT);
}

void Color::whiteRed()
{
  analogWrite(_redPIN, 0);
}

void Color::whiteRedDimmed()
{
  analogWrite(_redPIN, 980); // 1024 = off... 0 = full power .. So ... 900 should be very dimmed but running...
}

void Color::whiteGreen()
{
  analogWrite(_greenPIN, 0);
}

void Color::whiteGreenDimmed()
{
  analogWrite(_greenPIN, 980); // 1024 = off... 0 = full power .. So ... 900 should be very dimmed but running...
}

void Color::whiteBlueDimmed()
{
  analogWrite(_bluePIN, 980); // 1024 = off... 0 = full power .. So ... 900 should be very dimmed but running...
}

void Color::whiteBlue()
{
  analogWrite(_bluePIN, 0);
}

void Color::whiteBoth()
{
  analogWrite(_redPIN, 0);
  analogWrite(_greenPIN, 0);
  analogWrite(_bluePIN, 0);
}

void Color::off()
{
  analogWrite(_redPIN, 1024); // 1024 = Off
  analogWrite(_greenPIN, 1024);
  analogWrite(_bluePIN, 1024);
}

void Color::on()
{
  analogWrite(_redPIN, 0); // 1024 = Off
  analogWrite(_greenPIN, 0);
  analogWrite(_bluePIN, 0);
}

void Color::blink()
{
  int FADESPEED = 5;
  int FADE_LIMIT = 1024;
  int FADE_OFFSET = 10; // Adjust if strip does not go to 0

  for (int r = 0; r < FADE_LIMIT + 1; r++)
  {
    analogWrite(_redPIN, FADE_LIMIT - r);
    delay(FADESPEED);
  }

  // fade from yellow to green
  for (int r = FADE_LIMIT + 1; r > 0; r--)
  {
    analogWrite(_redPIN, FADE_LIMIT - r + FADE_OFFSET);
    delay(FADESPEED);
  }

  // fade from red to yellow
  for (int g = 0; g < FADE_LIMIT + 1; g++)
  {
    analogWrite(_greenPIN, FADE_LIMIT - g);
    delay(FADESPEED);
  }

  // fade from yellow to green
  for (int r = FADE_LIMIT + 1; r > 0; r--)
  {
    analogWrite(_greenPIN, FADE_LIMIT - r + FADE_OFFSET);
    delay(FADESPEED);
  }
}

void Color::blinkShort()
{
  int FADESPEED = 1; // Lower= Faster... Higher=Slower
  int FADE_LIMIT = 1024;
  int FADE_OFFSET = 10; // Adjust if strip does not go to 0

  for (int r = 0; r < FADE_LIMIT + 1; r++)
  {
    analogWrite(_redPIN, FADE_LIMIT - r);
    delay(FADESPEED);
  }

  // fade from yellow to green
  for (int r = FADE_LIMIT + 1; r > 0; r--)
  {
    analogWrite(_redPIN, FADE_LIMIT - r + FADE_OFFSET);
    delay(FADESPEED);
  }

  // fade from red to yellow
  for (int g = 0; g < FADE_LIMIT + 1; g++)
  {
    analogWrite(_greenPIN, FADE_LIMIT - g);
    delay(FADESPEED);
  }

  // fade from yellow to green
  for (int r = FADE_LIMIT + 1; r > 0; r--)
  {
    analogWrite(_greenPIN, FADE_LIMIT - r + FADE_OFFSET);
    delay(FADESPEED);
  }
}