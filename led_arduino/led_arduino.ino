#include "TLC_lib.h"
#include "lib8tion.h"

// temporary output vars for hues and RGB values of LEDs
uint16_t hues[20];
uint16_t RGBs[60];

void setup() 
{
  // initialize with specified dim scale (in this case, 1/8th power)
  TLC_init(2);
}

void loop()
{
  static int ledIndex = 0;
  
  TLC_singleLED(ledIndex++);
  TLC_write();
  
  if (ledIndex >= 72) ledIndex = 0;
  
  delay(1000);
}
