#include "TLC_lib.h"
#include "lib8tion.h"

uint16_t hues[20];
uint16_t RGBs[60];

int mode=2;

// digital pins for MSGEQ7 chip
// analog in is on A0
const int EQ_RST    = 6;
const int EQ_STROBE = 7;

// stored sensor vals, scaled as appropriate
uint16_t eqVals[7];
uint16_t accelVals[3];
uint16_t distVal = 0;
uint16_t swVal = 0;

#define SERIAL_EN 1

void setup() 
{
  // initialize with specified dim scale (in this case, 1/8th power)
  TLC_init(3);
  
  for (int i=0; i<20; i++)
    hues[i] = random16(360);
      
#ifdef SERIAL_EN
  Serial.begin(115200);
#endif
  
  // pulse reset pin of MSGEQ7 to initialize
  pinMode(EQ_RST, OUTPUT);
  pinMode(EQ_STROBE, OUTPUT);
  pinMode(A0, INPUT);
  digitalWrite(EQ_RST, HIGH);
  delay(50);
  digitalWrite(EQ_RST, LOW);
  delay(50);
}

// get EQ vals from MSGEQ7
void readEQ()
{
  for (int i=0; i<7; i++)
  {
    // first strobe, then read out, no need to hit reset it looks like
    digitalWrite(EQ_STROBE, HIGH);
    delayMicroseconds(50);
    digitalWrite(EQ_STROBE, LOW);
    delayMicroseconds(50);
    eqVals[i] = analogRead(A0);
  }
}



void loop()
{
  static uint16_t rgb[3];
  
  // read the sensors
  readEQ();
   
#ifdef SERIAL_EN
  // transmit sensor data
  // order is EQ, sel, accel, distance
  for (int i=0; i<7; i++)
  {
    Serial.print(eqVals[i],DEC);
    Serial.print(",");
  }
  Serial.print(swVal,DEC);
  Serial.print(",");
  for (int i=0; i<3; i++)
  {
    Serial.print(accelVals[i],DEC);
    Serial.print(",");
  }
  Serial.println(distVal,DEC);
#endif
  
  rgb[0] = eqVals[1]>>4;
  rgb[1] = eqVals[2]>>4;
  rgb[2] = eqVals[3]>>4;

#ifdef SERIAL_EN
  // check if we received bytes to set the color/brightness
  if (Serial.available() > 2)
  {
    mode = 2;
    // get all of the rgb values
    rgb[0] = Serial.read();
    rgb[1] = Serial.read();
    rgb[2] = Serial.read();
    // and scale them for gamma 2
    for (int i=0; i<2; i++)
      rgb[i] *= rgb[i];
    // flush buffer
    while (Serial.available() > 0)
      Serial.read();
  }
#endif

  switch (mode)
  {
    case 0:
      for (int i=0; i<20; i++)
      {
        hues[i]++;
        if (hues[i] >= 360)
          hues[i] = 0;
          
        HSVtoRGB(hues[i], 255, 255, RGBs+3*i);
      }
      TLC_setData(RGBs);
      break;
      
    case 2:
      TLC_setAll(rgb[0],rgb[1],rgb[2]);
      break;
  }

  TLC_write();
  delay(20);
}
