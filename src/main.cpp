
#include "LedControl.h"
#include "Keypad.h"
#include <Arduino.h>
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
#define D02 2

#define D03 3 //

//data / clk / cs
LedControl lc = LedControl(D12, D10, D11, 1);


//BombMachine bomb = BombMachine("8715003*");



void setup() {
  delay(100); //boot
  //wakeup call
  lc.shutdown(0, false);
  //Set the brightness to a medium values
  lc.setIntensity(0, 8);
  //and clear the display
  lc.clearDisplay(0);

  //randomSeed(analogRead(0));
}

void loop() {
  //char c = keypad.waitForKey();
  //lc.setChar(0, 0, c, false);  
  if(keypad.getKeys()) //something changed
  {
    for(int i = 0; i < LIST_MAX; i++) {
      if(keypad.key[i].kstate==PRESSED)
      {
        lc.setChar(0, 0, keypad.key[i].kchar, false);
        break;
      }
    }
  }
}