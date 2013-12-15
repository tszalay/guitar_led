#include "TLC_lib.h"
#include "lib8tion.h"

uint16_t hues[20];
uint16_t RGBs[60];
uint16_t vals[256];

int mode=2;

const int EQ_RST    = 6;
const int EQ_STROBE = 7;

#define SERIAL_EN 1

void setup() 
{
  // initialize with specified dim scale (in this case, 1/8th power)
  TLC_init(3);
  
  for (int i=0; i<20; i++)
    hues[i] = random16(360);
  
  //float gamma = 2;
    
  for (int i=0; i<256; i++)
  {
    vals[i] = i;
    vals[i] *= i;
    //float f = i/255.0f;
    //vals[i] = (uint16_t)(65535*pow(f,gamma));
  }
//  delay(2000);
#ifdef SERIAL_EN
  Serial.begin(115200);
#endif
  /*for (int i=0; i<256; i++)
  {
    Serial.print(i,DEC);
    Serial.print(" ");
    Serial.print(vals[i] >> 8,HEX);
    Serial.print("  ");
    Serial.println(vals[i]&255,HEX);
  }*/
  
  // pulse reset pin of MSGEQ7
  pinMode(EQ_RST, OUTPUT);
  pinMode(EQ_STROBE, OUTPUT);
  pinMode(A0, INPUT);
  digitalWrite(EQ_RST, HIGH);
  delay(50);
  digitalWrite(EQ_RST, LOW);
  delay(50);
}


void loop()
{
  static uint16_t rgb[3];
  
  // read MSGEQ7 chip
  // strobe reset first
/*  digitalWrite(EQ_RST, HIGH);
  delayMicroseconds(10);
  digitalWrite(EQ_RST, LOW);
  delayMicroseconds(100);*/
  // and then read middle three channels
  int eqVals[7];
  
  for (int i=0; i<7; i++)
  {
    digitalWrite(EQ_STROBE, HIGH);
    delayMicroseconds(50);
    digitalWrite(EQ_STROBE, LOW);
    delayMicroseconds(50);
    eqVals[i] = analogRead(A0);
#ifdef SERIAL_EN    
    Serial.print(eqVals[i],DEC);
    Serial.print(",");
#endif
  }
#ifdef SERIAL_EN
  Serial.print("0,0,0\n");
#endif
  
  rgb[0] = eqVals[1]>>4;
  rgb[1] = eqVals[2]>>4;
  rgb[2] = eqVals[3]>>4;

#ifdef SERIAL_EN  
  if (Serial.available() > 2)
  {
    mode = 2;
    rgb[0] = Serial.read();
    rgb[1] = Serial.read();
    rgb[2] = Serial.read();
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
      TLC_setAll_16(vals[rgb[0]],vals[rgb[1]],vals[rgb[2]]);
      break;
  }

  TLC_write();
  delay(20);
}
