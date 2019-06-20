#ifdef TEST
#include <ArduinoFake.h>
void loop() {}
void setup() {}
#else
#include <Arduino.h>
#include <binary.h>
#include <LedControl.h>
#include <Keypad.h>

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

BombMachine bomb = BombMachine("8715003*", nullptr);

unsigned long tickResolution = 100; //ms
unsigned long lastTickUpdate = 0;

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

  //improve random
  randomSeed(analogRead(0));
  delay(50);
  for (int i = 0; i < analogRead(0); i++)
    random();
}

bool handleKeypad();
bool handleButton();
void handleDisplay(unsigned long, bool);
void handleSpeaker();

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

  unsigned long updateMillis = millis();
  handleDisplay(updateMillis, externalUpdate);

  if (updateMillis - lastTickUpdate < tickResolution && !externalUpdate)
    return;

  bomb.tick(updateMillis - lastTickUpdate);
  handleSpeaker();
  lastTickUpdate = updateMillis;
}

void handleSpeaker()
{
  switch (bomb.getState())
  {
  case BombMachine::BombState::PrepareArming:
  case BombMachine::BombState::LockedArming:
  case BombMachine::BombState::Idle:
    //shutdown speaker
    noTone(A5);
    pinMode(A5, OUTPUT);
    analogWrite(A5, 0);
    break;
  case BombMachine::BombState::Armed:
  {
    unsigned long remaining = bomb.getRemainingBombTime();
    unsigned long total = bomb.getTotalBombTime();

    /*unsigned long actual = total - remaining;
    long freq = map(actual, 0, total, 0, 65535);
    if (freq > 100)
    {
      tone(A5, freq);
    }
    else
    {
      
      pinMode(A5, OUTPUT);
      analogWrite(A5, 255);
    }*/
    noTone(A5);
    analogWrite(A5, 255);

    break;
  }
  case BombMachine::BombState::PrepareDisarming:
  {
    break;
  }
  case BombMachine::BombState::Arming:
  case BombMachine::BombState::Disarming:
  {
    break;
  }
  case BombMachine::BombState::LockedDisarming:
  {
    break;
  }
  case BombMachine::BombState::Disarmed:
  {
    tone(A5, 50);
    break;
  }
  case BombMachine::BombState::Exploded:
  {
    tone(A5, 50);
    break;
  }
  default:
    break;
  }
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
bool buttonDebounce = false;
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
      buttonDebounce = false;
      lastButtonChange = millis();
      //bomb.pressButton();
      //return true;
    }
  }
  else
  {
    //no change but maybe we hold the button
    if (buttonPress && millis() - lastButtonChange > buttonHoldTime && !buttonDebounce)
    {
      buttonDebounce = true;
      bomb.pressButton();
      return true;
    }
  }
  return false;
}

unsigned long lastDisplayUpdate = 0;
unsigned long displayUpdateResolution = 2000;
unsigned long animationSpeed = 250;

inline void animateIdle();
inline void animateDisarmed();
inline void animateExploded();
inline void animateArmed();
inline void animateLocked();

void handleDisplay(unsigned long updateMillis, bool forceUpdate)
{
  //todo handle animations

  bool inAnimation =
      bomb.getState() == BombMachine::BombState::Idle ||
      bomb.getState() == BombMachine::BombState::Disarmed ||
      bomb.getState() == BombMachine::BombState::Exploded ||
      bomb.getState() == BombMachine::BombState::Armed ||
      bomb.getState() == BombMachine::BombState::LockedArming ||
      bomb.getState() == BombMachine::BombState::LockedDisarming;

  if (updateMillis - lastDisplayUpdate < displayUpdateResolution && !forceUpdate && !inAnimation)
    return;
  if (inAnimation && updateMillis - lastDisplayUpdate < animationSpeed && !forceUpdate)
    return;
  lastDisplayUpdate = updateMillis;

  switch (bomb.getState())
  {
  case BombMachine::BombState::Idle:
  {
    animateIdle();
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
    animateArmed();
    break;
  }
  case BombMachine::BombState::Disarmed:
  {
    animateDisarmed();
    break;
  }
  case BombMachine::BombState::Exploded:
  {
    animateExploded();
    break;
  }
  case BombMachine::BombState::LockedArming:
  case BombMachine::BombState::LockedDisarming:
  {
    animateLocked();
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

unsigned int idleAnimStep = 0;
const static byte idleAnimStates[] = {
    B00001001, //mid+bottom
    B01000001, //mid+top

    B01000011, //left+top
    B00001101, //left+bottom

    B00011001, //right+bottom
    B01100001, //right+top
};

inline void animateIdle()
{
  animationSpeed = 100;

  lc.setRow(0, 7, B00000101); //r
  lc.setRow(0, 6, B00111101); //d
  lc.setRow(0, 5, B00111011); //y

  lc.setRow(0, 4, idleAnimScrollStates[idleAnimStep == 0 ? 3 : idleAnimStep == 1 ? 4 : 0]);
  lc.setRow(0, 3, idleAnimScrollStates[idleAnimStep == 2 ? 1 : idleAnimStep == 9 ? 2 : 0]);
  lc.setRow(0, 2, idleAnimScrollStates[idleAnimStep == 3 ? 1 : idleAnimStep == 8 ? 2 : 0]);
  lc.setRow(0, 1, idleAnimScrollStates[idleAnimStep == 4 ? 1 : idleAnimStep == 7 ? 2 : 0]);
  lc.setRow(0, 0, idleAnimScrollStates[idleAnimStep == 5 ? 5 : idleAnimStep == 6 ? 6 : 0]);

  idleAnimStep = (idleAnimStep + 1) % 10;
}

inline void animateDisarmed()
{
  for (int i = 0; i < 8; i++)
  {
    lc.setChar(0, i, 'D', true);
  }
}
inline void animateExploded()
{
  for (int i = 0; i < 8; i++)
    lc.setChar(0, i, 'E', true);
}
const static byte armedAnimStates[] = {
    B00000111,
    B00000001,
    B00110001,
};
inline void animateArmed()
{
  lc.clearDisplay(0);
  lc.setRow(0, 5, B00001111); //t
  lc.setChar(0, 4, '-', true);

  unsigned long remBombTime = bomb.getRemainingBombTime() / 1000UL;

  unsigned long positions = log10(remBombTime) + 1;
  unsigned long currentPosition = remBombTime;

  if ( positions == 1 ){
    lc.setDigit(0, 3, (byte) 0, true);
    lc.setDigit(0, 2, (byte) 0, true);
  } 
  if ( positions == 2 ){
    lc.setDigit(0, 3, (byte) 0, true);
  }

  for (unsigned long i = 0; i < positions; i++ )
  {
    unsigned long currentDigit = currentPosition % 10UL;
    lc.setDigit(0, 1+i, (byte) currentDigit, true);
    currentPosition = currentPosition / 10UL;
  }

    lc.setRow(0, 7, armedAnimStates[0]);
    lc.setRow(0, 6, armedAnimStates[1]);
    lc.setRow(0, 0, armedAnimStates[2]);
}
unsigned int lockedAnimStep = 0;
const static byte lockedAnimStates[] = {
    B00000000,  //off
    B00000110, //left
    B00000111, //left+mid
    B00110000, //right
    B00110001, //right+mid
    B00000001,
};
inline void animateLocked()
{
  animationSpeed = 250;
  //lockdown in seconds
  unsigned long lockDownTime = bomb.getRemainingActionTime();
  if(lockDownTime < 1000 && lockDownTime > 0) {
    lockDownTime = 1;
  } else if(lockDownTime > 1000) {
    lockDownTime /= 1000UL;
    lockDownTime += 1;
  } else {
    lockDownTime = 0;
  }

  lc.setRow(0, 0, lockedAnimStep == 0 ? lockedAnimStates[0] : lockedAnimStep > 1 ? lockedAnimStates[4] : lockedAnimStates[3]); //right

  lc.setRow(0, 1, lockedAnimStep == 0 ? lockedAnimStates[0] : lockedAnimStep >= 2 ? lockedAnimStates[5] : lockedAnimStates[0]);
  lc.setRow(0, 2, lockedAnimStep == 0 ? lockedAnimStates[0] : lockedAnimStep >= 3 ? lockedAnimStates[5] : lockedAnimStates[0]);

  lc.setDigit(0, 3, lockDownTime % 10UL, true);
  lc.setDigit(0, 4, (lockDownTime / 10UL) % 10UL, true);

  lc.setRow(0, 5, lockedAnimStep == 0 ? lockedAnimStates[0] : lockedAnimStep >= 3 ? lockedAnimStates[5] : lockedAnimStates[0]);
  lc.setRow(0, 6, lockedAnimStep == 0 ? lockedAnimStates[0] : lockedAnimStep >= 2 ? lockedAnimStates[5] : lockedAnimStates[0]);

  lc.setRow(0, 7, lockedAnimStep == 0 ? lockedAnimStates[0] : lockedAnimStep > 1 ? lockedAnimStates[2] : lockedAnimStates[1]); //left

  lockedAnimStep = (lockedAnimStep + 1) % 5;
}

#endif