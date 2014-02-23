#include "TLC_lib.h"
#include "lib8tion.h"

void setup() 
{
  // initialize with specified dim scale (in this case, 1/8th power)
  TLC_init(4);
}

void loop()
{
  static int ledIndex = 0;
  
///  TLC_singleLED(1000);
///  TLC_write();
///  delay(3);
  
  TLC_singleLED(ledIndex++);
  TLC_write();
  
  if (ledIndex >= 48) ledIndex = 0;
  
  delay(500);
}

