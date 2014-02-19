#include "TLC_lib.h"
#include "lib8tion.h"

#include <SPI.h>
#include <ADXL362.h>

// update period, in ms
const uint16_t loopTime = 20;
// and frequency
const uint16_t loopFreq = 1000/loopTime;

// ADXL values
ADXL362 adxl;


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

// debouncing for the switch
uint16_t newSwVal     = 0;
uint16_t newSwTime    = 0;
const uint16_t dbTime = 200; // ms to wait on new value

// computed values
uint16_t curPower;        // sum of EQ spectra, full range
uint16_t avgPower;        // time-average (lowpass filter of previous)
const uint16_t pwrTau = 63000; // mixing fraction for averaging

// slow fading for mode switching
// gonna get here eventually



// computed counters and whatnot
// everything is full-range unless stated otherwise

// this is an array of random phases to use for dispersion
//uint16_t* randPhase = new uint16_t[20];
uint16_t randPhase[20];

// here be parameters/magic numbers (for corresponding mode):
const uint16_t m1_slowTime = 5;  // time to loop hue, in seconds
const uint8_t  m1_dispMag  = 100;  // magnitude of dispersion, in 255ths (sorry!)
const uint8_t  m1_brt      = 50;  // brightness of the mode
const uint16_t m1_dHue = (65535/m1_slowTime/loopFreq); // amount to add each iteration, is about 21
const uint16_t m1_dDisp    = 5*m1_dHue;  // rate at which dispersion changes

const uint16_t m2_pwrScale  = 1200; // scaling for power
const uint8_t  m2_dispScale = 2;    // how much faster dispersion changes than power

const uint16_t m3_pwrScale = 1200;   // scaling for power to fade btwn LEDs
uint8_t        m3_curTgt   = 0;     // which LED is fading
uint8_t        m3_nextTgt  = 10;    // which LED is brightening
uint16_t       m3_curHue   = 0;     // hue of current LED
uint16_t       m3_nextHue  = 20000; // hue of next LED
uint16_t       m3_fadeFac  = -1;    // fraction faded to next LED

uint16_t       m4_bright   = 0;     // current brightness of flashing
const uint16_t m4_decay    = 65000; // decay rate for flashy flash
const uint16_t m4_thresh   = 10000; // minimum value for power to make brighty

const uint16_t m5_hues[]   = {0, 5000, 10000, 15000, 20000};
uint16_t       m5_dthresh  = 10000;



// overall hue of the guitar. everybody must update, for smooth transitionssss
uint16_t avgHue    = 0;
// loop variable to use for dispersion
uint16_t dispVal   = 0;

// temporary output vars for hues and RGB values of LEDs
uint16_t hues[20];
uint16_t RGBs[60];



// are we transmitting data over serial?
#define SERIAL_EN 1


void setup() 
{
  // initialize with specified dim scale (in this case, 1/8th power)
  TLC_init(2);
  
  // initalize axdl
  adxl.begin();                   // Setup SPI protocol, issue device soft reset
  adxl.beginMeasure();            // Switch ADXL362 to measure mode  
  adxl.checkAllControlRegs();     // Burst Read all Control Registers, to check for proper setup

  randomSeed(analogRead(4));
  for (int i=0; i<20; i++)
    randPhase[i] = random(65535);
    
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
  int x,y,z,t;
  // read all three axis in burst to ensure all measurements correspond to same sample time
  adxl.readXYZTData(x, y, z, t);
  // it's 12 bits, so multiply by the missing 4 bits = 16
  accelVals[0] = x*16;
  accelVals[1] = y*16;
  accelVals[2] = t*16;
}

void readSwitch()
{
  int val = analogRead(A1);
  // calculate from 6-way voltage divider (5 resistors)
  int curSwitch = (val + 100)/205;
  // adding a quick little debounce thing here
  if (curSwitch == swVal)
  {
    // no change, do nothing
    return;
  }
  else if (curSwitch == newSwVal)
  {
    // still on new value, debouncing
    newSwTime += loopTime;
    // done debouncing?
    if (newSwTime > dbTime)
    {
      // update main switch value
      swVal = newSwVal;
    }
  }
  else
  {
    // start debouncing
    newSwTime = 0;
    newSwVal = curSwitch;
  }
}

// read sharp distance sensor
void readDistance()
{
  // val here is likely under 3.5V, or 716 (10-bit)
  // add 1 to avoid divide by zero
  // otherwise, sensor reports inverse distance
  // this scaling gives a pretty good range
  // and also keeps it from overflowing
  uint32_t val = (65535*100)/(1+analogRead(A4));
  if (val > 65535) val = 65535;
  // scale to 0...65535
  distVal = val;
}

// transmit sensor data, if we so desire
void sendSensors()
{
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
  Serial.print(distVal,DEC);
  // now print power and other relevant vars
  Serial.print(",");
  Serial.println(avgPower,DEC);
#endif
}


// set and disperse all hues, by the given phase and magnitude (out of 255)
void disperseHues(uint16_t hue, uint16_t phase, uint8_t mag)
{
  // assumes the hues array is populated, this modifies by disps
  for (int i=0; i<20; i++)
  {
    int16_t sval = sin16(phase+randPhase[i]);
    if (sval > 0)
      hues[i] = hue + scale16(abs(sval),20000U);
    else
      hues[i] = hue - scale16(abs(sval),20000U);
  }
}

// disperse all hues, by the given phase and magnitude (out of 255)
void disperseHues(uint16_t phase, uint8_t mag)
{
  disperseHues(0,phase,mag);
}

// convert preset H values to RGB with SV
void huesToRGB(uint8_t sat, uint8_t val)
{
  for (int i=0; i<20; i++)
    HSVtoRGB(scale16(hues[i],360), sat, val, RGBs+3*i);
}

void calcModes()
{
  swVal = 3;
  switch (swVal)
  {
    case 0:
    {
      // all LEDs off
      for (int i=0; i<60; i++)
        RGBs[i] = 0;
        
      TLC_setAll(0,0,0);
      break;
    }
    case 1:
    {
      // standard slow fade, minimal dispersion
      avgHue += m1_dHue;
      // add dispersion fade, faster than hue fade
//      dispVal += m1_dDisp;
      // disperse the hues
//      disperseHues(avgHue, 0, m1_dispMag);
      for (int i=0; i<20; i++)
        hues[i] = avgHue+randPhase[i];
        
      huesToRGB(255, m1_brt);
      
      TLC_setData(RGBs);
      break;
    }
    case 2:
    {
      // Fade rate modulated by power
      uint16_t pwr = scale16(avgPower, m2_pwrScale);
      avgHue += pwr;

      // add dispersion fade, faster than hue fade
      dispVal += m2_dispScale*pwr;

      // disperse the hues
      disperseHues(avgHue, dispVal, m1_dispMag);
      huesToRGB(255, m1_brt);

      TLC_setData(RGBs);
      break;
    }
    case 3:
    {
      // Jumping lights, from one region to another
      uint16_t pwr = scale16(avgPower, m3_pwrScale);
      pwr = m3_pwrScale*2;
      m3_fadeFac += pwr;
      // did we just wrap around?
      if (m3_fadeFac < pwr)
      {
        // cycle current and next onesies, and pick randomly
        m3_curTgt = m3_nextTgt;
        m3_curHue = m3_nextHue;
//        while (m3_nextTgt == m3_curTgt)
          m3_nextTgt = m3_curTgt + 1;//random(20);
          if (m3_nextTgt > 19) m3_nextTgt = 0;
        m3_nextHue = random(65535);
        // also start at 0
        m3_fadeFac = 0;
      }
      // set all dem RGBs to 0, in case of previous thingies
      memset(RGBs,0,2*60);
      // now set jumping LEDs
      uint8_t ff = m3_fadeFac >> 8;
      m3_curHue = m3_nextHue = 0;
//      HSVtoRGB(scale16(m3_curHue,360), 255, 255-ff, RGBs+3*m3_curTgt);
//      HSVtoRGB(scale16(m3_nextHue,360), 255, ff, RGBs+3*m3_nextTgt);
//      HSVtoRGB(scale16(3000,360), 255, 255, RGBs+3*4);
//      RGBs[14] = 65535;
      RGBs[32] = 65535;
      
      TLC_setData(RGBs);
      break;
    }
    case 4:
    {
      // Sound blaster! Flash white on big'n events
      if (avgPower > m4_thresh)
        m4_bright = avgPower;
      else
        m4_bright = scale16(avgPower, m4_decay);

      // move dispersion along
      dispVal += m1_dDisp;
      
      // run the full gamut of hues for this dispersion
      disperseHues(0, dispVal, 255);
      
      // but when we set the RGBs we use the sat
      huesToRGB(127-(m4_bright >> 9), m4_bright >> 8);

      TLC_setData(RGBs);
      break;
    }
    case 5:
    {
      // Guitar on fire mode
      
      dispVal += m1_dDisp;
      
      // set hue based on distance and avg. power
      // interpolate and blah blah blah
      uint16_t pwr = (distVal>>1) + (avgPower>>1);
      uint8_t pind = pwr >> 14; // 0...3, use ind
      uint16_t hue = lerp16by16(m5_hues[pind], m5_hues[pind], pwr & ((1<<14)-1));
      // adjust intensity as well
      uint8_t brt = 0;
      if (distVal > m5_dthresh)
        brt = pwr >> 9 + 128;
  
      disperseHues(hue, dispVal, brt>>4);
      huesToRGB(255, brt);
      
      TLC_setData(RGBs);
      break;
    }
  }
}




void loop()
{
  // start of calculations and stuff, in us
  uint32_t tstart = micros();
  
  // read the sensors, takes about 2 ms
  readEQ();
  readAccel();
  readSwitch();
  readDistance();
  
  // also takes 1 ms or something like that
  sendSensors();

  // can put lots of calculations here then, if you want
  calcModes();
  
  // and finally, write the newest data to the LED drivers
  // (takes about 1.5 ms)
  TLC_write();
  
  // and total time taken
  uint32_t dt = micros() - tstart;
  // make up the rest of the time with a delay, if needed (?)
//  if (1000*loopTime > dt)
//    delayMicroseconds(1000*loopTime - dt);
  delay(30);
}
