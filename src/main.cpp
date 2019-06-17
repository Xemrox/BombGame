#include <Arduino.h>

#include <LedControl.h>
#include <Keypad.h>
//#include <HardwareSerial.h>
#include "bombmachine.h"

#define D12 12 //data
#define D11 11 //cs
#define D10 10 //clk

#define D09 9 //1
#define D08 8 //
#define D07 7 //

#define D06 6 //
#define D05 5 //
#define D04 4 //
#define D03 3 //

#define D02 2 //Button

#define Button D02

#define A7 //
#define A6 //
#define A05 19//
#define A4 //

//data / clk / cs
LedControl lc = LedControl(D12, D10, D11, 1);

const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}};
byte rowPins[ROWS] = {D06, D05, D04, D03}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {D09, D08, D07};      //connect to the column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

BombMachine bomb = BombMachine("8715003*");

unsigned long lastDisplayUpdate = 0;
bool idleAnim = false;

void setup()
{
  delay(100); //boot
  //wakeup call
  lc.shutdown(0, false);
  //Set the brightness to a medium values
  lc.setIntensity(0, 14);
  //and clear the display
  lc.clearDisplay(0);

  //initialize button
  pinMode(Button, INPUT);
  //pullup
  digitalWrite(Button, 1);

  //randomSeed(analogRead(0));

  pinMode(A05, OUTPUT);

  lastDisplayUpdate = millis();
}

void handleKeypad();
void handleButton();

void loop()
{
  //tone(A05, 6000, 1000);
  //analogWrite(A05, 1023);
  //analogWrite(A05, 0);

  //char c = keypad.waitForKey();
  //lc.setChar(0, 0, c, false);
  handleKeypad();
  handleButton();

  unsigned long updateMillis = millis();
  if (updateMillis - lastDisplayUpdate < 100UL)
    return;
  bomb.tick(updateMillis - lastDisplayUpdate);

  lastDisplayUpdate = millis();

  switch (bomb.getState())
  {
  case BombMachine::BombState::Idle:
  {
    char idle0 = ' ';
    char idle1 = ' ';

    if (idleAnim)
    {
      idle0 = '-';
      idle1 = '_';
    }
    else
    {
      idle0 = '_';
      idle1 = '-';
    }
    idleAnim = !idleAnim;

    for (int i = 0; i < 8; i += 2)
    {
      lc.setChar(0, i, idle0, false);
    }
    for (int i = 1; i < 8; i += 2)
    {
      lc.setChar(0, i, idle1, false);
    }
    break;
  }
  case BombMachine::BombState::PrepareArming:
  case BombMachine::BombState::PrepareDisarming:
  {
    lc.clearDisplay(0);
    const char *code = bomb.getBombCode();
    for (int i = 0; i < bomb.getBombCodeSize(); i++)
    {
      lc.setChar(0, 7 - i, code[i], false);
    }


    /*for (int i = bomb.getBombCodeSize() - 1; i < 8; i++)
    {
      lc.setChar(0, 7 - i, ' ', false);
    }*/
    break;
  }
  case BombMachine::BombState::Disarming:
  case BombMachine::BombState::Arming:
  {
    lc.clearDisplay(0);
    const char *code = bomb.getKeyBuffer();
    for (int i = 0; i < bomb.getKeyPosition(); i++)
    {
      if (code[i] == '\0')
      {
        lc.setChar(0, 7 - i, ' ', false);
      }
      else
      {
        lc.setChar(0, 7 - i, code[i], true);
      }
    }
    /*for (int i = bomb.getKeyPosition(); i < 8; i++)
    {
      lc.setChar(0, 7 - i, ' ', false);
    }*/
    break;
  }
  case BombMachine::BombState::Configuring:
  {
    //lc.clearDisplay(0);
    lc.setChar(0, 7, 'C', false);
    break;
  }
  case BombMachine::BombState::Configuration:
  {
    //lc.clearDisplay(0);
    lc.setChar(0, 7, 'C', true);
    break;
  }
  case BombMachine::BombState::Armed:
  {
    for (int i = 0; i < 8; i++)
    {
      lc.setChar(0, i, 'X', true);
    }
    break;
  }
  case BombMachine::BombState::Disarmed:
  {
    for (int i = 0; i < 8; i++)
    {
      lc.setChar(0, i, 'D', true);
    }
    break;
  }
  case BombMachine::BombState::Exploded:
  {
    for (int i = 0; i < 8; i++)
    lc.setChar(0, 7, 'E', true);
    break;
  }
  default:
  {
    for (int i = 0; i < 8; i++)
    lc.setChar(0, 8, 'U', true);
    break;
  }
  }
}

void handleKeypad() {
  if (keypad.getKeys()) //something changed
  {
    for (int i = 0; i < LIST_MAX; i++)
    {
      if (keypad.key[i].kstate == PRESSED && keypad.key[i].stateChanged)
      {
        //lc.setChar(0, 0, keypad.key[i].kchar, false);
        bomb.inputKey(keypad.key[i].kchar);
        break;
      }
    }
  }
}

bool previousButtonState = false;
void handleButton() {
  bool buttonPress = !digitalRead(D02); //LOW active
  if(buttonPress != previousButtonState) {
    previousButtonState = buttonPress;
    if(buttonPress) {
      bomb.pressButton();
    }
  }
}