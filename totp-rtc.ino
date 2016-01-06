#include <Wire.h>
#include <RTClib.h>
// #include <swRTC.h>
#include <Keypad.h>
#include <sha1.h>
#include <TOTP.h>

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
byte rowPins[ROWS] = {12,11,10,9}; // pins from 12 to 9
byte colPins[COLS] = {8,7,6,5}; // pins from 8 to 5
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

RTC_DS1307 rtc;
// swRTC rtc;
char* totpCode;
char inputCode[7];
int inputCode_idx;

boolean doorOpen;

int ledError = 2;
int ledDoor = 3;
int output = 13;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  rtc.begin();

  DEBUG_PRINTLN("TOTP Door lock");
  DEBUG_PRINTLN("");

  // reset input buffer index
  inputCode_idx = 0;

  // close the door
  doorOpen = false;
  DEBUG_PRINTLN("Door closed");

  pinMode(ledError, OUTPUT);
  pinMode(ledDoor, OUTPUT);
}

void loop () {
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

          if(doorOpen == true) DEBUG_PRINTLN("Code ok but the door is already open");

          else {
            DEBUG_PRINTLN("Code ok, opening the door...");
            digitalWrite(ledDoor, HIGH);
            digitalWrite(output, HIGH);
            doorOpen = true;
            delay(15000);
            DEBUG_PRINTLN("Time out, closing the door...");
            digitalWrite(ledDoor, LOW);
            digitalWrite(output, LOW);
            doorOpen = false;
          }

        // code is wrong :(
        } else {
          DEBUG_PRINT("Wrong code... the correct was: ");
          DEBUG_PRINTLN(totpCode);
          digitalWrite(ledError, HIGH);
          delay(2000);
          digitalWrite(ledError, LOW);
        }
      }
    }
  }
}
