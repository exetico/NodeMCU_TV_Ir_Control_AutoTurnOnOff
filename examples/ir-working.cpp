#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <NTPClient.h>   // https://github.com/arduino-libraries/NTPClient
#include <WiFiUdp.h>
#include <ezButton.h>

// PIN CONFIGURATION
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>

// IR
const uint16_t kIrLed = 13; // D7 = GPIO 13
const uint16_t kRecvPin = 15; // D8 = GPIO 15

IRsend irsend(kIrLed);
IRrecv irrecv(kRecvPin);

decode_results results;


// VARS
//-- WIFI
bool __WIFI_CONNECTED = false;

//-- CONFIG
bool fetching_config = false;

//-- TV STATE
bool __TV_IS_ON = 0;
// int __TV_TRIGGER_EPOCH_NEXT = 0;
// int __TV_TRIGGER_EPOCH_PREVIOUS = 0;

int __TV_ON_SEC = 0;
int __TV_OFF_SEC = 0;

//-- CONTROL IR - Default values if config is not fetch
int __C_TURN_ON_HH = 7;
int __C_TURN_ON_MM = 55;
int __C_TURN_ON_SS = 30;
int __C_TURN_OFF_HH = 16;
int __C_TURN_OFF_MM = 30;
int __C_TURN_OFF_SS = 30;

//-- TIME
int __THIS_TIME = 0;
int __TIME_HH = 0;
int __TIME_MM = 0;
int __TIME_SS = 0;
int __TIME_EPOCH = 0;
String __TIME_FORMATTED = "";

//-- BUTTONS
ezButton button(4); // GPIO4 = D2 // OBS
unsigned long lastCount = 0;
unsigned long count = 0;
unsigned long buttonIsPressed = 0;

unsigned long lastButtonTriggerLongPressMillis = 3000;
unsigned long lastButtonPressMillis = millis();
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

String HTTPS_PATH = "/playground/nodemcu-tv-ir-control_turn-on-off.json";
String HTTPS_START = "https://";
String CONFIG_HOST = "tobiasnordahl.dk";
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

          __C_TURN_ON_HH = int(obj["config"]["turnOn"]["hh"]);
          __C_TURN_ON_MM = int(obj["config"]["turnOn"]["mm"]);
          __C_TURN_ON_SS = int(obj["config"]["turnOn"]["ss"]);
          __C_TURN_OFF_HH = int(obj["config"]["turnOff"]["hh"]);
          __C_TURN_OFF_MM = int(obj["config"]["turnOff"]["mm"]);
          __C_TURN_OFF_SS = int(obj["config"]["turnOff"]["ss"]);

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
    Serial.println("Turning tv off");
  }
  else if (turnTvOn)
  {
    __TV_IS_ON = 1;
    Serial.println("Turning tv on");
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

  __TV_ON_SEC = (__C_TURN_ON_HH * 60 * 60) + (__C_TURN_ON_MM * 60) + (__C_TURN_ON_SS);
  __TV_OFF_SEC = (__C_TURN_OFF_HH * 60 * 60) + (__C_TURN_OFF_MM * 60) + (__C_TURN_OFF_SS);

  // Turn tv off
  if (__THIS_TIME > __TV_OFF_SEC && __TV_IS_ON)
  {
    Serial.println("Turning tv off");
    controlTvState(0);
  }
  else if (__THIS_TIME > __TV_ON_SEC && __THIS_TIME < __TV_OFF_SEC && !__TV_IS_ON)
  { // Turn tv on
    Serial.println("Turning tv on");
    controlTvState(1);
  }
  else
  { // I'm not sure... BUt something

    Serial.println("I'm not sure");
  }
}

void blinkESP12Led()
{
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
}

void checkOneSecInterval()
{
  checkTvState();
}

// SETUP
void setup()
{

  Serial.println("Starter op..");

  irsend.begin();

  #if ESP8266
    Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  #else  // ESP8266
    Serial.begin(115200, SERIAL_8N1);
  #endif // ESP8266

  Serial.begin(115200);
  irrecv.enableIRIn();  // Start the receiver
  while (!Serial)  // Wait for the serial connection to be establised.
    delay(50);
  Serial.println();
  Serial.print("IRrecvDemo is now running and waiting for IR message on Pin ");
  Serial.println(kRecvPin);

  /*

  // Txt
  Serial.println("Starter op..");

  // Use internal LED
  pinMode(D4, OUTPUT); 

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

    // Blink LED
    blinkESP12Led();

    Serial.println("Calling timeClient");
    timeClient.begin();

    // Download file
    Serial.println("Download begin");
    get_json_config();

    if (__C_TURN_ON_HH && __C_TURN_OFF_HH)
    {
      Serial.println("Download OK");
    }
    Serial.println("Download end");

    checkOneSecInterval();
  }
  else
  {
    Serial.println("Configuration portal running");
  }

  */
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

// Example of data captured by IRrecvDumpV2.ino
uint16_t rawData[67] = {9000, 4500, 650, 550, 650, 1650, 600, 550, 650, 550,
                        600, 1650, 650, 550, 600, 1650, 650, 1650, 650, 1650,
                        600, 550, 650, 1650, 650, 1650, 650, 550, 600, 1650,
                        650, 1650, 650, 550, 650, 550, 650, 1650, 650, 550,
                        650, 550, 650, 550, 600, 550, 650, 550, 650, 550,
                        650, 1650, 600, 550, 650, 1650, 650, 1650, 650, 1650,
                        650, 1650, 650, 1650, 650, 1650, 600};
// Example Samsung A/C state captured from IRrecvDumpV2.ino
uint8_t samsungState[kSamsungAcStateLength] = {
    0x02, 0x92, 0x0F, 0x00, 0x00, 0x00, 0xF0,
    0x01, 0xE2, 0xFE, 0x71, 0x40, 0x11, 0xF0};

// Samsung Turn Off/On = DF7BB480


  static unsigned long atime2, btime2, ctime2;
void loop()
{
  if (delay_without_delaying(btime2, 100))
  {
      // Every 100
      if (irrecv.decode(&results)) {
        // print() & println() can't handle printing long longs. (uint64_t)
        serialPrintUint64(results.value, HEX);
        Serial.println("");
        irrecv.resume();  // Receive the next value
      }
  }


  if (delay_without_delaying(ctime2, 5000))
  {
    // Every 2 sec
    // Serial.println("NEC");
    // irsend.sendNEC(0x00FFE01FUL);
    // delay(1000);
    // Serial.println("Sony");
    // irsend.sendSony(0xa90, 12, 2); // 12 bits & 2 repeats
    // delay(1000);
    Serial.println("a rawData capture from IRrecvDumpV2");
    irsend.sendRaw(rawData, 67, 38); // Send a raw data capture at 38kHz.
    delay(1000);
    Serial.println("a Samsung A/C state from IRrecvDumpV2");
    irsend.sendSamsungAC(samsungState);
    delay(1000);
  }


  

}

String command;
void loop2()
{

  // Read serial
  if (Serial.available())
  {
    command = Serial.readStringUntil('\n');

    Serial.printf("Command received %s \n", command.c_str());

    char *commandStr = const_cast<char *>(command.c_str());

    // Serial.println(strtok(commandStr, "bob"));
    // Serial.println(command.indexOf("bob"));
    // Serial.println(command.equals("bob"));

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
  }

  if (button.isReleased())
  {
    Serial.println("The button is released");
    buttonIsPressed = 0;
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

    int countIn6 = count % 6 + 1;

    lastButtonPressMillis = millis();

    switch (countIn6)
    {
    case 1:
      Serial.println("Case 1, woop");
      break;

    case 2:
      Serial.println("Case 2, woop");
      break;

    case 3:
      Serial.println("Case 3, woop");
      break;

    case 4:
      Serial.println("Case 4, woop");
      break;

    case 5:
      Serial.println("Case 5, woop");
      checkOneSecInterval();
      break;

    case 6:
      Serial.println("Case 6, woop");
      __TV_IS_ON = !__TV_IS_ON;
      break;
    }

    lastCount = count;
  }

  // Loop, different timeperiods
  static unsigned long ledtime = 0;
  static unsigned long atime, btime, ctime;

  if (delay_without_delaying(ledtime, 500))
  {
    // Check if button press has been too long time ago
    unsigned long currentButtonmillis = millis();
    if (button.getCount() > 0 && currentButtonmillis - lastButtonPressMillis >= lastButtonTriggerLongPressMillis)
    {
      // Reset count
      button.resetCount();
      Serial.print("Reset Button Count");

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
