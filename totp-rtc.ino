#include <Wire.h>
#if defined(ESP8266)
#include <pgmspace.h>
#else
#include <avr/pgmspace.h>
#endif
#include <RtcDS3231.h>
#include <Keypad.h>
#include <sha1.h>
#include <TOTP.h>

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
byte rowPins[ROWS] = {11,10,9,8};
byte colPins[COLS] = {7,6,5,4};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

RtcDS3231 Rtc;
char* totpCode;
char inputCode[7];
int inputCode_idx;

boolean doorOpen;
boolean ledErrorActive = false;
boolean ledOkActive = false;

int ledError = 12;
int ledOk = 13;
int ledDoor = 3;
int countError = 0;
int countOk = 0;
int countDoor = 0;
int limitDoor = 300;
int limitLed = 100;

void setup() {
  Serial.begin(57600);

  //--------RTC SETUP ------------
  Rtc.Begin();
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

  if (!Rtc.IsDateTimeValid()) {
    // Common Cuases:
    //   1) first time you ran and the device wasn't running yet
    //   2) the battery on the device is low or even missing
    Serial.println("RTC lost confidence in the DateTime!");
    // following line sets the RTC to the date & time this sketch was compiled
    // it will also reset the valid flag internally unless the Rtc device is
    // having an issue
    Rtc.SetDateTime(compiled);
  }

  if (!Rtc.GetIsRunning()) {
    Serial.println("RTC was not actively running, starting now");
    Rtc.SetIsRunning(true);
  }

  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled) {
    Serial.println("RTC is older than compile time!  (Updating DateTime)");
    Rtc.SetDateTime(compiled);
  } else if (now > compiled) {
    Serial.println("RTC is newer than compile time. (this is expected)");
  } else if (now == compiled) {
    Serial.println("RTC is the same as compile time! (not expected but all is fine)");
  }

  // never assume the Rtc was last configured by you, so
  // just clear them to your needed state
  Rtc.Enable32kHzPin(false);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);

  // reset input buffer index
  inputCode_idx = 0;

  // close the door
  doorOpen = false;

  pinMode(ledOk, OUTPUT);
  pinMode(ledError, OUTPUT);
  pinMode(ledDoor, OUTPUT);
}

void loop () {
  if (!Rtc.IsDateTimeValid()) {
    // Common Cuases:
    //   1) the battery on the device is low or even missing and the power line was disconnected
    Serial.println("RTC lost confidence in the DateTime!");
  }

  // counter as delay
  if (doorOpen == true) {
    if (countDoor > limitDoor) {
      digitalWrite(ledDoor, LOW);
      doorOpen = false;
      countDoor = 0;
    }
    countDoor++;
  }

  if (ledErrorActive == true) {
    if (countError > limitLed) {
      digitalWrite(ledError, LOW);
      ledErrorActive = false;
      countError = 0;
    }
    countError++;
  }

  if (ledOkActive == true) {
    if (countOk > limitLed) {
      digitalWrite(ledOk, LOW);
      ledOkActive = false;
      countOk = 0;
    }
    countOk++;
  }

  char key = keypad.getKey();

  // a key was pressed
  if (key != NO_KEY) {

    // # resets the input buffer
    if(key == '#') {
      inputCode_idx = 0;
    } else if (key == '*') {
      // * closes the door
      if (doorOpen == true) {
        digitalWrite(ledDoor, LOW);
        doorOpen = false;
      }
    } else {
      // save key value in input buffer
      inputCode[inputCode_idx++] = key;

      // if the buffer is full, add string terminator, reset the index
      // get the actual TOTP code and compare to the buffer's content
      if (inputCode_idx == 6) {
        inputCode[inputCode_idx] = '\0';
        inputCode_idx = 0;

        RtcDateTime now = Rtc.GetDateTime();
        long GMT = now.Epoch32Time();
        totpCode = totp.getCode(GMT);

        if (strcmp(inputCode, totpCode) == 0) {
          // code is ok :)
          if (ledErrorActive == true) {
            digitalWrite(ledError, LOW);
            ledErrorActive = false;
          }

          digitalWrite(ledOk, HIGH);
          ledOkActive = true;

          if (doorOpen == false) {
            digitalWrite(ledDoor, HIGH);
            doorOpen = true;
          }
        } else {
          // code is wrong :(
          if (ledOkActive == true) {
            digitalWrite(ledOk, LOW);
            ledOkActive = false;
          }

          digitalWrite(ledError, HIGH);
          ledErrorActive = true;
        }
      }
    }
  }

  delay(100);
}
