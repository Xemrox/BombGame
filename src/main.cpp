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

#define A7    //
#define A6    //
#define A5 19 //
#define A4    //

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

unsigned long tickResolution = 100; //ms
unsigned long lastTickUpdate = 0;

bool idleAnim = false;

void setup()
{
  delay(100); //boot
  //wakeup call
  lc.shutdown(0, false);
  //Set the brightness to a high values
  lc.setIntensity(0, 14);
  //and clear the display
  lc.clearDisplay(0);

  //initialize button
  pinMode(Button, INPUT);
  //pullup
  digitalWrite(Button, 1);
  pinMode(A5, OUTPUT);

  randomSeed(analogRead(0));
}

bool handleKeypad();
bool handleButton();
void handleDisplay(bool);

void loop()
{
  //tone(A5, 6000, 1000);
  //analogWrite(A5, 1023);
  //analogWrite(A5, 0);

  //char c = keypad.waitForKey();
  //lc.setChar(0, 0, c, false);
  bool keyUpdate = handleKeypad();
  bool buttonUpdate = handleButton();

  bool externalUpdate = keyUpdate || buttonUpdate;

  handleDisplay(externalUpdate);

  unsigned long updateMillis = millis();
  if (updateMillis - lastTickUpdate < tickResolution && !externalUpdate)
    return;

  bomb.tick(updateMillis - lastTickUpdate);
  lastTickUpdate = updateMillis;
}

bool handleKeypad()
{
  if (keypad.getKeys()) //something changed
  {
    for (int i = 0; i < LIST_MAX; i++)
    {
      if (keypad.key[i].kstate == PRESSED && keypad.key[i].stateChanged)
      {
        //lc.setChar(0, 0, keypad.key[i].kchar, false);
        bomb.inputKey(keypad.key[i].kchar);
        return true;
      }
    }
  }
  return false;
}

bool previousButtonState = false;
unsigned long buttonHoldTime = 1000;
unsigned long lastButtonChange = 0;
bool handleButton()
{
  bool buttonPress = !digitalRead(D02); //LOW active
  //debounce button change
  if (buttonPress != previousButtonState)
  {
    previousButtonState = buttonPress;
    if (buttonPress)
    {
      lastButtonChange = millis();
      //bomb.pressButton();
      //return true;
    }
  }
  else
  {
    //no change but maybe we hold the button
    if (buttonPress && millis() - lastButtonChange > buttonHoldTime)
    {
      bomb.pressButton();
      return true;
    }
  }
  return false;
}

unsigned long lastDisplayUpdate = 0;
unsigned long displayUpdateResolution = 5000;
void handleDisplay(bool forceUpdate)
{
  //todo handle animations

  unsigned long updateMillis = millis();
  if (updateMillis - lastDisplayUpdate < displayUpdateResolution && !forceUpdate)
    return;
  lastDisplayUpdate = updateMillis;

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
    break;
  }
  case BombMachine::BombState::Disarming:
  case BombMachine::BombState::Arming:
  {
    lc.clearDisplay(0);
    const char *code = bomb.getKeyBuffer();
    for (int i = 0; i < bomb.getKeyPosition(); i++)
    {
      lc.setChar(0, 7 - i, code[i], true);
    }
    break;
  }
  case BombMachine::BombState::Configuring:
  {
    lc.clearDisplay(0);
    lc.clearDisplay(0);
    const char *code = bomb.getKeyBuffer();
    for (int i = 0; i < bomb.getKeyPosition(); i++)
    {
      lc.setChar(0, 7 - i, code[i], true);
    }
    lc.setChar(0, 0, 'C', false);
    break;
  }
  case BombMachine::BombState::Configuration:
  {
    lc.clearDisplay(0);
    const char *code = bomb.getKeyBuffer();
    for (int i = 0; i < bomb.getKeyPosition(); i++)
    {
      lc.setChar(0, 7 - i, code[i], true);
    }
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
      lc.setChar(0, i, 'E', true);
    break;
  }
  default:
  {
    for (int i = 0; i < 8; i++)
      lc.setChar(0, i, 'U', true);
    break;
  }
  }
}