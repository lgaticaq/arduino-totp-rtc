#include "Wire.h"
#include "RTClib.h"
// #include "swRTC.h"
#include "Keypad.h"
#include "sha1.h"
#include "TOTP.h"

// debug print, use #define DEBUG to enable Serial output
// thanks to http://forum.arduino.cc/index.php?topic=46900.0
#define DEBUG
#ifdef DEBUG
  #define DEBUG_PRINT(x)  Serial.print(x)
  #define DEBUG_PRINTLN(x)  Serial.println(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif

// shared secret is arduinouno
uint8_t hmacKey[] = {0x61, 0x72, 0x64, 0x75, 0x69, 0x6e, 0x6f, 0x75, 0x6e, 0x6f};
TOTP totp = TOTP(hmacKey, 10);

// keypad configuration
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {9,8,7,6}; // pins from 9 to 6
byte colPins[COLS] = {5,4,3,2}; // pins from 5 to 2
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

RTC_DS1307 rtc;
// swRTC rtc;
char* totpCode;
char inputCode[7];
int inputCode_idx;

boolean doorOpen;

int ledError = 13;
int ledDoor = 12;

void setup() {
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while(1);
  }

  if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  Serial.begin(9600);

  DEBUG_PRINTLN("TOTP Door lock");
  DEBUG_PRINTLN("");

  // // init software RTC with the current time
  // rtc.stopRTC();
  // rtc.setDate(21, 10, 2015);
  // rtc.setTime(17, 41, 00);
  // rtc.startRTC();
  // DEBUG_PRINTLN("RTC initialized and started");

  // reset input buffer index
  inputCode_idx = 0;

  // close the door
  doorOpen = false;
  DEBUG_PRINTLN("Door closed");

  pinMode(ledError, OUTPUT);
  pinMode(ledDoor, OUTPUT);
}

void loop () {
  // DateTime now = rtc.now();
  // Serial.print(now.year(), DEC);
  // Serial.print('/');
  // Serial.print(now.month(), DEC);
  // Serial.print('/');
  // Serial.print(now.day(), DEC);
  // Serial.print(' ');
  // Serial.print(now.hour(), DEC);
  // Serial.print(':');
  // Serial.print(now.minute(), DEC);
  // Serial.print(':');
  // Serial.print(now.second(), DEC);
  // Serial.println();
  // delay(1000);

  char key = keypad.getKey();

  // a key was pressed
  if (key != NO_KEY) {

    // # resets the input buffer
    if(key == '#') {
      DEBUG_PRINTLN("# pressed, resetting the input buffer...");
      inputCode_idx = 0;
    }

    // * closes the door
    else if(key == '*') {

      if(doorOpen == false) DEBUG_PRINTLN("* pressed but the door is already closed");

      else {

        DEBUG_PRINTLN("* pressed, closing the door...");
        digitalWrite(ledDoor, LOW);
        doorOpen = false;
      }
    }
    else {

      // save key value in input buffer
      inputCode[inputCode_idx++] = key;

      // if the buffer is full, add string terminator, reset the index
      // get the actual TOTP code and compare to the buffer's content
      if(inputCode_idx == 6) {

        inputCode[inputCode_idx] = '\0';
        inputCode_idx = 0;
        // DEBUG_PRINT("New code inserted: ");
        // DEBUG_PRINTLN(inputCode);

        // long GMT = rtc.getTimestamp();
        DateTime now = rtc.now();
        long GMT = now.unixtime();
        totpCode = totp.getCode(GMT);
        // DEBUG_PRINT("New code generate: ");
        // DEBUG_PRINTLN(totpCode);

        // code is ok :)
        if(strcmp(inputCode, totpCode) == 0) {
          digitalWrite(ledError, LOW);

          if(doorOpen == true) DEBUG_PRINTLN("Code ok but the door is already open");

          else {
            DEBUG_PRINTLN("Code ok, opening the door...");
            digitalWrite(ledDoor, HIGH);
            doorOpen = true;
            delay(10000);
            DEBUG_PRINTLN("Time out, closing the door...");
            digitalWrite(ledDoor, LOW);
            doorOpen = false;
          }

        // code is wrong :(
        } else {
          DEBUG_PRINT("Wrong code... the correct was: ");
          DEBUG_PRINTLN(totpCode);
          digitalWrite(ledError, HIGH);
        }
      }
    }
  }
}
