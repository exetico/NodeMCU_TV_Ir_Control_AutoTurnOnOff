#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <NTPClient.h>   // https://github.com/arduino-libraries/NTPClient
#include <WiFiUdp.h>
#include <ezButton.h>

// IR Configuration
#include "PinDefinitionsAndMore.h" // Define macros for input and output pin etc.
#include <IRremote.hpp>

// Prevent Stack crashed for network-requests - See https://github.com/esp8266/Arduino/issues/6811
#include <StackThunk.h>
#define _stackSize (6900/4)

// VARS
//-- WIFI
bool __WIFI_CONNECTED = false;

//-- IR COMMANDS
uint16_t __IR_ADDRESS = 0x707;
uint8_t __IR_COMMAND_TURN_OFF = 0x98; // Turn Off
uint8_t __IR_COMMAND_TURN_ON = 0x99;  // Turn On
uint8_t __IR_REPEATS = 10;

//-- CONFIG
bool fetching_config = false;
bool __SHOW_POWER_STATE = false;
bool __CURRENT_STATE_LED = 0;
//-- TV STATE
bool __TV_IS_ON = false;
bool __BLUE_DIMMED = false;
// int __TV_TRIGGER_EPOCH_NEXT = 0;
// int __TV_TRIGGER_EPOCH_PREVIOUS = 0;

//-- TMP
bool __DOWNLOAD_CONFIG_OK = false;

int __TV_ON_SEC = 0;
int __TV_OFF_SEC = 0;

//-- CONTROL IR - Default values if config is not fetch
int __C_TURN_ON_HH = 7;
int __C_TURN_ON_MM = 55;
int __C_TURN_ON_SS = 30;
int __C_TURN_OFF_HH = 16;
int __C_TURN_OFF_MM = 30;
int __C_TURN_OFF_SS = 30;
//const char* __C_ALLOWED_DAYS = "None";
//String __C_ALLOWED_DAYS = String("None");
//char*  __C_ALLOWED_DAYS = (char*)"";
const char* __C_ALLOWED_DAYS = "";

//-- TIME
int __THIS_TIME = 0;
int __TIME_HH = 0;
int __TIME_MM = 0;
int __TIME_SS = 0;
int __TIME_WEEKDAY = 0;
int __TIME_EPOCH = 0;
String __TIME_FORMATTED = "";

//-- RGB Strips
#include "Color.h"
const int redLED = 2;   // D4 GPIO2   ...  D4 = GPIO2   .... GPIO0 = D3 BUT THAT'S USED FOR BUILD IN FLASH BUTTON!
const int greenLED = 14; // D5 GPIO14
const int blueLED = 12; // D6 GPIO12
Color createColor(redLED, greenLED, blueLED);

//-- BUTTONS
ezButton button(0); // GPIO4 = D2 // OBS .... // GPIO 13 = D7 .... GPIO 0 = Build in "Flash button" on NodeMCU
unsigned long lastCount = 0;
unsigned long count = 0;
unsigned long buttonIsPressed = 0;

unsigned long lastButtonTriggerLongPressMillis = 3000;
unsigned long lastButtonPressMillis = millis();
unsigned long previousButtonPressMillis = millis();
bool buttonPressedBefore = 0;
unsigned long lastButtonPressWasLongPress = 0;

// Time
const long utcOffsetInSeconds = 3600;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// Define NTP client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

// Download files
#include <WiFiClient.h>
WiFiClientSecure client;
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
/* Import: See https://randomnerdtutorials.com/esp32-http-get-post-arduino/#comment-722499*/
#include <ArduinoJson.h>

// Personal host
// String HTTPS_PATH = "/playground/nodemcu-tv-ir-control_turn-on-off.json";
// String HTTPS_START = "https://";
// String CONFIG_HOST = "tobiasnordahl.dk";

// Cloudflare worker -- https://micro.tobiasnordahl.dk/nodemcu-001-tv-ir-control.json // http://micro.tobiasnordahl.dk/nodemcu-001-tv-ir-control.json
String HTTPS_PATH = "/nodemcu-001-tv-ir-control.json";
String HTTPS_START = "https://";
String CONFIG_HOST = "micro.tobiasnordahl.dk";

// {"config":{"turnOn":{"hh":7, "mm":45, "ss": 05},"turnOff":{"hh":16, "mm":30, "ss": 05}}}

String CONFIG_URL = String(HTTPS_START) + String(CONFIG_HOST) + String(HTTPS_PATH);
const char *URL_COMPLETE = CONFIG_URL.c_str();

// WIFI - WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
WiFiManager wm;

void log_i(const String &txt, int configInt)
{
  Serial.println(" ");
  Serial.print(txt + " ");
  Serial.print(String(configInt));
  Serial.println(" ");
}

int digitalReadOutputPin(uint8_t pin)
{
  uint8_t bit = digitalPinToBitMask(pin);
  uint8_t port = digitalPinToPort(pin);
  if (port == NOT_A_PIN)
    return LOW;

  return (*portOutputRegister(port) & bit) ? HIGH : LOW;
}

void turnOnLED(bool turnOn, bool returnToPowerState = 0, int delayInt = 500){
  //   int state;
  //   state = digitalReadOutputPin(D4);
  // turnOnLED(false, 1);
  if(turnOn){
   digitalWrite(D4, __SHOW_POWER_STATE); 
  }else{
    digitalWrite(D4, !__SHOW_POWER_STATE);
  }

  // Return to power state LED
  if(returnToPowerState){
    delay(delayInt);
    if(__SHOW_POWER_STATE){
      digitalWrite(D4, LOW);
    }else{
      digitalWrite(D4, HIGH);
    }
  }
}

void blinkESP8266LED(int delayMs, int loopAmount)
{
  while (loopAmount > 0)
  {

    turnOnLED(true);
    delay(delayMs);
    turnOnLED(false);
    if (loopAmount > 1)
    {
      delay(delayMs);
    }
    loopAmount = loopAmount - 1;
  }

  turnOnLED(false, 1); // Return to previous state
}

bool fetch_json_config(void)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    String url = CONFIG_URL;

    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
    client->setInsecure();
    HTTPClient https;

    if (https.begin(*client, CONFIG_URL))
    { // HTTPS
      Serial.println("[HTTPS] GET...");
      int httpCode = https.GET();

      // httpCode will be negative on error
      if (httpCode > 0)
      {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
        // file found at server?
        if (httpCode == HTTP_CODE_OK)
        {
          String payload = https.getString();
          Serial.println(String("[HTTPS] Received payload: ") + payload);

          DynamicJsonDocument doc(1024);
          DeserializationError error = deserializeJson(doc, payload);
          JsonObject obj = doc.as<JsonObject>();

          if (error)
          {
            Serial.print("deserializeJson() failed: ");
            Serial.println(error.c_str());
            return 1;
          }

          __DOWNLOAD_CONFIG_OK = true;

          __C_TURN_ON_HH = int(obj["config"]["turnOn"]["hh"]);
          __C_TURN_ON_MM = int(obj["config"]["turnOn"]["mm"]);
          __C_TURN_ON_SS = int(obj["config"]["turnOn"]["ss"]);
          __C_TURN_OFF_HH = int(obj["config"]["turnOff"]["hh"]);
          __C_TURN_OFF_MM = int(obj["config"]["turnOff"]["mm"]);
          __C_TURN_OFF_SS = int(obj["config"]["turnOff"]["ss"]);
          __C_ALLOWED_DAYS = obj["config"]["allowedDays"]; // obj["config"]["allowedDays"].as<char*>()

          log_i("__C_TURN_ON_HH", __C_TURN_ON_HH);
          log_i("__C_TURN_ON_MM", __C_TURN_ON_MM);
          log_i("__C_TURN_ON_SS", __C_TURN_ON_SS);
          log_i("__C_TURN_OFF_HH", __C_TURN_OFF_HH);
          log_i("__C_TURN_OFF_MM", __C_TURN_OFF_MM);
          log_i("__C_TURN_OFF_SS", __C_TURN_OFF_SS);

          return 0;
        }
      }
      else
      {
        Serial.printf("[HTTPS] GET... failed, error: %s\n\r", https.errorToString(httpCode).c_str());

        return 0;
      }

      https.end();
    }
    else
    {
      Serial.printf("[HTTPS] Unable to connect\n\r");
    }

    return 0;
  }
  else
  {
    Serial.println("Wifi Err");
    return 1;
  }
}

void get_json_config(void)
{
  fetching_config = true;
  while (fetching_config)
  {
    fetching_config = fetch_json_config();
  }
}

void controlTvState(boolean turnTvOn)
{
  if (!turnTvOn)
  {
    __TV_IS_ON = 0;
    Serial.println("Turning tv off 1/3");
    blinkESP8266LED(600, 3);
    IrSender.sendSamsung(__IR_ADDRESS, __IR_COMMAND_TURN_OFF, __IR_REPEATS);
    Serial.println("Turning tv off 2/3");
    blinkESP8266LED(600, 3);
    IrSender.sendSamsung(__IR_ADDRESS, __IR_COMMAND_TURN_OFF, __IR_REPEATS);
    Serial.println("Turning tv off 3/3");
    blinkESP8266LED(600, 3);
    IrSender.sendSamsung(__IR_ADDRESS, __IR_COMMAND_TURN_OFF, __IR_REPEATS);
    createColor.off();
    createColor.whiteRedDimmed();
  }
  else if (turnTvOn)
  {
    __TV_IS_ON = 1;
    Serial.println("Turning tv on 1/3");
    blinkESP8266LED(150, 5);
    IrSender.sendSamsung(__IR_ADDRESS, __IR_COMMAND_TURN_ON, __IR_REPEATS);
    Serial.println("Turning tv on 2/3");
    blinkESP8266LED(150, 5);
    IrSender.sendSamsung(__IR_ADDRESS, __IR_COMMAND_TURN_ON, __IR_REPEATS);
    Serial.println("Turning tv on 3/3");
    blinkESP8266LED(150, 5);
    IrSender.sendSamsung(__IR_ADDRESS, __IR_COMMAND_TURN_ON, __IR_REPEATS);
    createColor.off();
    createColor.whiteGreenDimmed();
  }
}

void checkTvState()
{
  Serial.println("");
  //Serial.println("__TV_TRIGGER_EPOCH_NEXT " + String(__TV_TRIGGER_EPOCH_NEXT));
  //Serial.println("__TV_TRIGGER_EPOCH_PREVIOUS " + String(__TV_TRIGGER_EPOCH_PREVIOUS));
  Serial.print("__TIME_EPOCH " + String(__TIME_EPOCH));
  Serial.println("  __C_TURN_ON_HH : MM : SS " + String(__C_TURN_ON_HH)) + " : " + String(__C_TURN_ON_MM) + " : " + String(__C_TURN_ON_SS);
  Serial.println("  __C_TURN_OFF_HH : MM : SS " + String(__C_TURN_OFF_HH)) + " : " + String(__C_TURN_OFF_MM) + " : " + String(__C_TURN_OFF_SS);
  Serial.println("  __C_ALLOWED_DAYS " + String(__C_ALLOWED_DAYS));
  Serial.println("  __THIS_TIME " + String(__THIS_TIME) + "__TV_OFF_SEC" + String(__TV_OFF_SEC));
  

  __TV_ON_SEC = (__C_TURN_ON_HH * 60 * 60) + (__C_TURN_ON_MM * 60) + (__C_TURN_ON_SS);
  __TV_OFF_SEC = (__C_TURN_OFF_HH * 60 * 60) + (__C_TURN_OFF_MM * 60) + (__C_TURN_OFF_SS);

  // Weekday not in scope, so do nothing
  if(!(String(__C_ALLOWED_DAYS).indexOf(String(__TIME_WEEKDAY)) != -1)){
    Serial.println("Weekday not in scope");
    Serial.print("Weekday: ");
    Serial.println(daysOfTheWeek[__TIME_WEEKDAY]);

    Serial.print("Allowed days (sun0, mon1, tues2, wed3, thur4, fri5, sat6): ");
    Serial.println("__C_ALLOWED_DAYS " + String(__C_ALLOWED_DAYS));


      if (!__BLUE_DIMMED)
      {
        __BLUE_DIMMED = 1;
        createColor.off();
        createColor.whiteBlueDimmed();
      }
  }
  // Trigger turn off
  else if (__THIS_TIME > __TV_OFF_SEC && __TV_IS_ON)
  {
    __BLUE_DIMMED = 0;
    Serial.println("Turning tv off");
    controlTvState(0);
  }
  // Trigger turn on
  else if (__THIS_TIME > __TV_ON_SEC && __THIS_TIME < __TV_OFF_SEC)
  { // Turn tv on
    __BLUE_DIMMED = 0;
    Serial.println("Turning tv on");
    controlTvState(1);
  }
  // Nothing to do right now
  else
  {
    // Should mean that nothing should be done
    Serial.println("All good, nothing to do");
    Serial.print("Weekday: ");
    Serial.println(daysOfTheWeek[__TIME_WEEKDAY]);
  }
}


void checkOneSecInterval()
{
  checkTvState();
}

// SETUP
void setup()
{

  // LED on ESP8266 chip (internal LED)
  pinMode(D4, OUTPUT);

  Serial.begin(115200);

  while (!Serial) // Wait for the serial connection to be establised.
    delay(50);

  Serial.println("Starter op..");
  createColor.whiteBlue();
  delay(500);
  createColor.off();
  createColor.whiteBlue();
  delay(500);
  createColor.off();


  // Set Current state LED
  __CURRENT_STATE_LED = digitalReadOutputPin(D4);

  // IR Init
  Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_IRREMOTE));
  IrSender.begin(DISABLE_LED_FEEDBACK); // Start with IR_SEND_PIN as send pin and disable feedback LED at default feedback LED pin
  Serial.println(IR_SEND_PIN);

  // Txt
  Serial.println("Starter op..");

  // Buttons
  button.setDebounceTime(50); // set debounce time to 50 milliseconds
  button.setCountMode(COUNT_FALLING);

  // put your setup code here, to run once:
  Serial.begin(115200);

  // WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  // it is a good practice to make sure your code sets wifi mode how you want it.

  // reset settings - wipe stored credentials for testing
  // these are stored by the esp library
  // wm.resetSettings();

  // Automatically connect using saved credentials,
  // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
  // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
  // then goes into a blocking loop awaiting configuration and will return success result

  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  res = wm.autoConnect("AutoConnectAP", "standupnow"); // password protected ap

  if (!res)
  {
    Serial.println("Failed to connect");
    // ESP.restart();
  }

  if (res)
  {
    __WIFI_CONNECTED = true;

    // if you get here you have connected to the WiFi
    Serial.println("Connected...Wuhu :)");
    createColor.whiteBlue();

    // Blink LED
    blinkESP8266LED(300, 6);

    Serial.println("Calling timeClient");
    timeClient.begin();

    // Download file
    Serial.println("Download begin");

    createColor.whiteRed();

    get_json_config();

    if (__C_TURN_ON_HH && __C_TURN_OFF_HH && __DOWNLOAD_CONFIG_OK)
    {
      Serial.println("Download OK");

      createColor.off();
      createColor.whiteBlue();
      createColor.whiteGreen();

      Serial.println("__C_TURN_ON_HH");
      Serial.println(__C_TURN_ON_HH);
      Serial.println("__C_TURN_OFF_HH");
      Serial.println(__C_TURN_OFF_HH);
      Serial.println("Download end");
      blinkESP8266LED(200, 6);

      createColor.off();
      createColor.whiteGreen();
      checkOneSecInterval();
    }else{
      createColor.off();
      createColor.whiteRed();
      Serial.println("Download end");
      Serial.println("Will not continue due to invalid download");
      blinkESP8266LED(1000, 20);

    }
    Serial.println("Download end");


  }
  else
  {
    createColor.off();
    createColor.whiteRed();
    Serial.println("Configuration portal running");
  }
}

// Delays
boolean delay_without_delaying(unsigned long time)
{
  // return false if we're still "delaying", true if time ms has passed.
  // this should look a lot like "blink without delay"
  static unsigned long previousmillis = 0;
  unsigned long currentmillis = millis();
  if (currentmillis - previousmillis >= time)
  {
    previousmillis = currentmillis;
    return true;
  }
  return false;
}

boolean delay_without_delaying(unsigned long &since, unsigned long time)
{
  // return false if we're still "delaying", true if time ms has passed.
  // this should look a lot like "blink without delay"
  unsigned long currentmillis = millis();
  if (currentmillis - since >= time)
  {
    since = currentmillis;
    return true;
  }
  return false;
}

String command;
void loop()
{

  // Read serial
  if (Serial.available())
  {
    command = Serial.readStringUntil('\n');

    Serial.printf("Command received %s \n", command.c_str());

    char *commandStr = const_cast<char *>(command.c_str());

    if (command.indexOf("sn") == (0))
    {
      Serial.println(strtok(commandStr, "sn"));
      String hh_mm_ss = strtok(commandStr, "sn");
      __C_TURN_ON_HH = atol(hh_mm_ss.substring(0, 1).c_str());
      __C_TURN_ON_MM = atol(hh_mm_ss.substring(2, 3).c_str());
      __C_TURN_ON_SS = atol(hh_mm_ss.substring(4, 5).c_str());
    }

    if (command.indexOf("on") == (0))
    {
      Serial.println("tv on");
      controlTvState(1);
    }

    if (command.indexOf("off") == (0))
    {
      Serial.println("tv off");
      controlTvState(0);
    }

    if (command.indexOf("1") == (0))
    {
      Serial.println("!!!1");
      IrSender.sendSamsung(__IR_ADDRESS, __IR_COMMAND_TURN_OFF, __IR_REPEATS);
      delay(1000);
    }

    if (command.indexOf("2") == (0))
    {
      Serial.println("!!!2");
      IrSender.sendSamsung(__IR_ADDRESS, __IR_COMMAND_TURN_ON, __IR_REPEATS);
      delay(1000);
    }

    if (command.indexOf("3") == (0))
    {
      Serial.println("!!!3");
      IrSender.sendSamsung(__IR_ADDRESS, __IR_COMMAND_TURN_OFF, __IR_REPEATS);
      delay(2000);
      IrSender.sendSamsung(__IR_ADDRESS, __IR_COMMAND_TURN_ON, __IR_REPEATS);
      delay(2000);
    }
  }

  if (wm.getConfigPortalActive())
  {
    Serial.println("No wifi1....");
  }

  if (wm.getWebPortalActive())
  {
    Serial.println("No wifi2....");
  }

  // Button counts
  button.loop(); // MUST call the loop() function first

  if (button.isPressed())
  {
    Serial.println("The button is pressed");
    buttonIsPressed = 1;
    turnOnLED(!digitalReadOutputPin(D4));
  }

  if (button.isReleased())
  {
    Serial.println("The button is released");
    buttonIsPressed = 0;
    turnOnLED(!digitalReadOutputPin(D4));
  }

  // If no wifi, skip the rest...
  if (!__WIFI_CONNECTED)
  {
    return;
  }

  if (lastButtonPressWasLongPress)
  {
    // Not long press tigger anymore, and fade light to confirm userinput
    lastButtonPressWasLongPress = !lastButtonPressWasLongPress;

    Serial.println(".. .. LONG PRESS .. ..");

    if (__TV_IS_ON)
    {
      Serial.println("LONG :( TURNING TV OFF");
      controlTvState(0);
    }
    else if (!__TV_IS_ON)
    {
      Serial.println("LONG :) TURNING TV ON");
      controlTvState(1);
    }
  }

  count = button.getCount();

  if (count != lastCount)
  {
    Serial.println(count);

    // If button has not been pressed before
    if (!buttonPressedBefore)
    {
      lastButtonPressMillis = millis();
    }

    previousButtonPressMillis = lastButtonPressMillis;
    lastButtonPressMillis = millis();

    lastCount = count;
  }

  // Loop, different timeperiods
  static unsigned long ledtime = 0;
  static unsigned long atime, btime, ctime;

  if (delay_without_delaying(ledtime, 500))
  {
    // Check if button press has been too long time ago
    unsigned long currentButtonmillis = millis();
    count = button.getCount();
    if (count > 0 && currentButtonmillis - lastButtonPressMillis >= lastButtonTriggerLongPressMillis)
    {
      Serial.println("Long-press millis triggered - Now running through cases with switch");

      // Check the count
      switch (count)
      {
      case 1:
        __SHOW_POWER_STATE = !__SHOW_POWER_STATE;

        Serial.println("Case 1, Toogle LED");
        turnOnLED(true, true, 100);

        break;

      case 2:
        if (millis() > lastButtonPressMillis + 1000)
        {
          Serial.println("Case 2, Get state");
          if (__TV_IS_ON)
          {
            blinkESP8266LED(150, 5);
          }
          else
          {
            blinkESP8266LED(600, 3);
          }
        }
        break;

      case 3:
        Serial.println("Case 3, __IR_COMMAND_TURN_OFF");
        blinkESP8266LED(600, 4);
        IrSender.sendSamsung(__IR_ADDRESS, __IR_COMMAND_TURN_OFF, __IR_REPEATS);
        break;

      case 4:
        Serial.println("Case 4, __IR_COMMAND_TURN_ON");
        blinkESP8266LED(150, 9);
        IrSender.sendSamsung(__IR_ADDRESS, __IR_COMMAND_TURN_ON, __IR_REPEATS);
        break;

      case 5:
        Serial.println("Case 5, checkOneSecInterval");
        checkOneSecInterval();
        break;

      case 6:
        Serial.println("Case 6, __TV_IS_ON");
        __TV_IS_ON = !__TV_IS_ON;
        if (__TV_IS_ON)
        {
          blinkESP8266LED(150, 8);
          IrSender.sendSamsung(__IR_ADDRESS, __IR_COMMAND_TURN_ON, __IR_REPEATS);
        }
        else
        {
          blinkESP8266LED(600, 3);
          IrSender.sendSamsung(__IR_ADDRESS, __IR_COMMAND_TURN_OFF, __IR_REPEATS);
        }
        break;
      }

      // Reset count
      button.resetCount();
      Serial.println("Reset Button Count - And running through switch-cases");

      // Set state of longpress button
      if (buttonIsPressed)
        lastButtonPressWasLongPress = 1;
    }
  }
  // Every 1000
  if (delay_without_delaying(atime, 1000))
  {
    // Update time
    timeClient.update();
    __TIME_HH = timeClient.getHours();
    __TIME_MM = timeClient.getMinutes();
    __TIME_SS = timeClient.getSeconds();
    __TIME_EPOCH = timeClient.getEpochTime();
    __TIME_FORMATTED = timeClient.getFormattedTime();
    __THIS_TIME = (__TIME_HH * 60 * 60) + (__TIME_MM * 60) + __TIME_SS;

    // Calculate weekday
    __TIME_WEEKDAY = ( (__TIME_EPOCH / 86400L) + 4 ) % 7; // Januar 1, 1970 was a Thursday, 0 will be Thursday if you use "day % 7". Instead use "(day+4) % 7" if you need sunday to be the first day of the week

    // Check every sec
    checkOneSecInterval();
  }
  if (delay_without_delaying(btime, 5000))
  {
    // Nothing right now - Serial.print("B");
  }
  if (delay_without_delaying(ctime, 500))
  {
    // Nothing right now - Serial.print("C");
  }
}
