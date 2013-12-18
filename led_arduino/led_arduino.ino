#include "TLC_lib.h"
#include "lib8tion.h"

#include <SPI.h>
#include <ADXL362.h>


uint16_t hues[20];
uint16_t RGBs[60];

int mode=2;

// ADXL values
ADXL362 adxl;
int temp;
int XValue, YValue, ZValue, Temperature;



// digital pins for MSGEQ7 chip
// analog in is on A0
const int EQ_RST    = 6;
const int EQ_STROBE = 7;

// min and max values for the analong MSGEQ input
// to make input data be full range
const uint16_t EQ_MIN = 80;
const uint16_t EQ_MAX = 1023;

// stored sensor vals, scaled as appropriate
uint16_t eqVals[7];       // full range
int16_t accelVals[3];     // full range
uint16_t distVal = 0;     // full range
uint16_t swVal = 0;       // 0...5

// computed values
uint16_t curPower;        // sum of EQ spectra, full range
uint16_t avgPower;        // time-average of previous
const uint8_t pwrTau = 65000; // mixing fraction for averaging, in 255ths


#define SERIAL_EN 1

void setup() 
{
  // initialize with specified dim scale (in this case, 1/8th power)
  TLC_init(3);
  
  // initalize axdl
  adxl.begin();                   // Setup SPI protocol, issue device soft reset
  adxl.beginMeasure();            // Switch ADXL362 to measure mode  
  adxl.checkAllControlRegs();     // Burst Read all Control Registers, to check for proper setup

  
  for (int i=0; i<20; i++)
    hues[i] = random16(360);
    
  // initialize all non-instantaneous values
  avgPower = 0;
      
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
  uint16_t pwr = 0;
  
  for (int i=0; i<7; i++)
  {
    // first strobe, then read out, no need to hit reset it looks like
    digitalWrite(EQ_STROBE, HIGH);
    delayMicroseconds(50);
    digitalWrite(EQ_STROBE, LOW);
    delayMicroseconds(50);
    // get the 10-bit raw analog value
    uint16_t val = analogRead(A0);
    // clamp and subtract
    val = constrain(val,EQ_MIN,EQ_MAX) - EQ_MIN;
    // rescale to full 16-bit range
    eqVals[i] = val*(65535/(EQ_MAX-EQ_MIN));
    // and add value to current power
    pwr += scale16(eqVals[i],65535/7);
  }

  // update current and running avg  
  curPower = pwr;
  avgPower = scale16(avgPower,pwrTau)+scale16(curPower,65535-pwrTau);
}

void readAccel()
{
  // read all three axis in burst to ensure all measurements correspond to same sample time
  adxl.readXYZTData(XValue, YValue, ZValue, Temperature);
  // it's 12 bits, so multiply by the missing 4 bits = 16
  accelVals[0] = XValue*16;
  accelVals[1] = YValue*16;
  accelVals[2] = ZValue*16;
}

void readSwitch()
{
  int val = analogRead(A1);
  swVal = (val + 100)/205;
}

void readDistance()
{
  // val here is likely under 3.5V, or 716 (10-bit)
  uint32_t val = (65535*100)/(1+analogRead(A4));
  if (val > 65535) val = 65535;
  // scale to 1...65535
  distVal = val;
}


void loop()
{
  static uint16_t rgb[3];
  
  // read the sensors, takes about 2 ms
  readEQ();
  readAccel();
  readSwitch();
  readDistance();
  
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
