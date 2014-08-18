#include "TLC_lib.h"
#include "lib8tion.h"
#include "mode_defs.h"

#include <SPI.h>
#include <ADXL362.h>


// ADXL values
ADXL362 adxl;

// vertical orientation for accelerometer
int16_t accMid_X = -15750;
int16_t accMid_Y = 5100;
int16_t accMid_Z = 3750;

const int hitThresh = 8000;


// digital pins for MSGEQ7 chip
const int EQ_RST    = 7;
const int EQ_STROBE = 8;

// min and max values for the analong MSGEQ input
// to make input data be full range
const uint16_t EQ_MIN = 82;
const uint16_t EQ_MAX = 1023;

// stored sensor vals, scaled as appropriate
uint16_t eqVals[7];       // full range
int16_t accelVals[3];     // full range
uint16_t swVal = 0;       // 0...5

// debouncing for the switch
uint16_t newSwVal     = 0;
uint16_t newSwTime    = 0;
const uint16_t dbTime = 200; // ms to wait on new value

// computed values
uint16_t curPower;                 // sum of EQ spectra, full range
uint16_t avgPower;                 // time-average (lowpass filter of previous)
const uint8_t numPrevPower = 4;
uint16_t prevPower[numPrevPower];  // previous _average_ power values (for peak finding)
uint8_t  prevPowerInd=0;           // index into rotating buffer for above
const uint16_t pwrRise = 63000;    // decay (averaging) rate for power going up
const uint16_t pwrFall = 64000;    // decay rate for power going down


// computed counters and whatnot
// everything is full-range unless stated otherwise

// this is an array of random phases to use for dispersion
uint16_t randPhase[16];


// overall hue of the guitar. everybody must update, for smooth transitionssss
uint16_t avgHue    = 0;
// loop variable to use for dispersion
uint16_t dispVal   = 0;

// temporary output vars for hues and RGB values of LEDs
uint16_t hues[16];
uint16_t RGBs[48];



// are we transmitting data over serial?
#define SERIAL_EN 0


void setup() 
{
  // initialize with specified dim scale (0 is full power)
  TLC_init(0);
  
  // initalize axdl
  adxl.begin();                   // Setup SPI protocol, issue device soft reset
  adxl.beginMeasure();            // Switch ADXL362 to measure mode  
  adxl.checkAllControlRegs();     // Burst Read all Control Registers, to check for proper setup

  randomSeed(analogRead(4));
  for (int i=0; i<16; i++)
    randPhase[i] = random(65535);
    
  // initialize all non-instantaneous values
  avgPower = 0;
  
  for (int i=0; i<numPrevPower; i++)
    prevPower[i] = 0;
  
  // set the mode
  readSwitch();
  swVal = newSwVal;
      
#ifdef SERIAL_EN
  Serial.begin(115200);
#endif

  for (int i=0; i<7; i++)
    eqVals[i] = 0;
  
  // pulse reset pin of MSGEQ7 to initialize
  pinMode(EQ_RST, OUTPUT);
  pinMode(EQ_STROBE, OUTPUT);
  pinMode(A3, INPUT);
  digitalWrite(EQ_RST, HIGH);
  delay(5);
  digitalWrite(EQ_RST, LOW);
  delay(5);
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
    // don't read the last one, it's dumb
    if (i==6)
      break;
    // get the 10-bit raw analog value
    uint16_t val = analogRead(A3);
    // clamp and subtract
    val = constrain(val,EQ_MIN,EQ_MAX) - EQ_MIN;
    // rescale to full 16-bit range
    eqVals[i] = val*(65535/(EQ_MAX-EQ_MIN));
    // and add value to current power
    pwr += scale16(eqVals[i],65535/7);
  }
  
  digitalWrite(EQ_RST, HIGH);
  delayMicroseconds(50);
  digitalWrite(EQ_RST, LOW);
  delayMicroseconds(50);


  // update current and running avg
  // (running avg has fast rise/slow decay)
  curPower = pwr;
  if (curPower > avgPower)
    avgPower = scale16(avgPower,pwrRise)+scale16(curPower,65535-pwrRise);
  else
    avgPower = scale16(avgPower,pwrFall)+scale16(curPower,65535-pwrFall);
  // and save into rotating buffer
  prevPowerInd = (prevPowerInd+1)&(numPrevPower-1);
  prevPower[prevPowerInd] = curPower;
}

void readAccel()
{
  int16_t x,y,z,t;
  // read all three axis in burst to ensure all measurements correspond to same sample time
  adxl.readXYZTData(x, y, z, t);
  // it's 12 bits, so multiply by the missing 4 bits = 16
  x *= 16;
  y *= 16;
  z *= 16;
  // test for sudden hit
  if (abs(x-accelVals[0]) > hitThresh || abs(y-accelVals[1]) > hitThresh || abs(z-accelVals[2]) > hitThresh)
  {
    avgPower = 65535;
  }
  accelVals[0] = x;
  accelVals[1] = y;
  accelVals[2] = z;
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

// transmit sensor data, if we so desire
void sendSensors()
{
#ifdef SERIAL_EN
  // transmit sensor data
  // order is EQ, sel, accel, power
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
  // now print power and other relevant vars
  Serial.print(avgPower,DEC);
  Serial.print(",");
  Serial.print(curPower,DEC);
  Serial.print("\n");
#endif
}


// set and disperse all hues, by the given phase and magnitude (out of 255)
void disperseHues(uint16_t hue, uint16_t phase, uint16_t mag)
{
  // assumes the hues array is populated, this modifies by disps
  for (int i=0; i<16; i++)
  {
    int16_t sval = sin16(phase+randPhase[i]);
    if (sval > 0)
      hues[i] = hue + scale16(abs(sval),mag);
    else
      hues[i] = hue - scale16(abs(sval),mag);
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
  for (int i=0; i<16; i++)
    HSVtoRGB(scale16(hues[i],360), sat, val, RGBs+3*i);
}

void calcModes()
{
  switch (swVal)
  {
    case 0:
    {
      // all LEDs off
      //memset(RGBs,0,2*48);
      
      //TLC_setAll(0,0,0);
      TLC_setAll(0xFFFFU,0xFFFFU,0xFFFFU);
      break;
    }
    case 1:
    {
      // standard slow fade, minimal dispersion
      avgHue += m1_dHue;
      // add dispersion fade, faster than hue fade
      dispVal += m1_dDisp;
      // disperse the hues
      disperseHues(avgHue, dispVal, m1_dispMag);
      
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
      disperseHues(avgHue, dispVal, avgPower/2);
      huesToRGB(255, m1_brt);

      TLC_setData(RGBs);
      break;
    }
    case 3:
    {
      // Jumping lights, from one region to another
      uint16_t pwr = scale16(avgPower, m3_pwrScale);
      pwr += m3_baseSpd;
      m3_fadeFac += pwr;
      // did we just wrap around?
      if (m3_fadeFac < pwr)
      {
        // cycle current and next onesies, and pick randomly
        m3_curTgt = m3_nextTgt;
        m3_curHue = m3_nextHue;
        while (m3_nextTgt == m3_curTgt)
          m3_nextTgt = random(16);

        m3_nextHue = random(65535);
        // also start at 0
        m3_fadeFac = 0;
      }
      // set all dem RGBs to 0, in case of previous thingies
      memset(RGBs,0,2*48);
      // now set jumping LEDs
      uint8_t ff = m3_fadeFac >> 8;
      HSVtoRGB(scale16(m3_curHue,360), 255, 255-ff, RGBs+3*m3_curTgt);
      HSVtoRGB(scale16(m3_nextHue,360), 255, ff, RGBs+3*m3_nextTgt);
      
      TLC_setData(RGBs);
      break;
    }
    case 4:
    {
      // Sound blaster! Flash white on big'n events
      
      uint16_t pwr = curPower;
      if (pwr < m4_toff)
        pwr = 0;
        
      if (pwr > m4_power)
        m4_power = scale16(m4_power,pwrRise)+scale16(pwr,65535-pwrRise);
      else
        m4_power = scale16(m4_power,m4_decay)+scale16(pwr,65535-m4_decay);
            
      // Check for a rapid change in current power
      uint8_t iprev = prevPowerInd;
      uint8_t iprev2 = (prevPowerInd-2)&(numPrevPower-1);

      // jump up, regardless of where we are, if we have a sudden spike in power      
      if (prevPower[iprev] > prevPower[iprev2] && prevPower[iprev] > prevPower[iprev2] + m4_ton)
      {
        m4_wub = 32768;
        m4_power = 65535;
      }

      m4_wub += m4_wubstep;
      // get cosine of wub fading value
      uint16_t val = cos16(m4_wub);
      // shift it by half, to go up and down, and scale down by 1/2
      val += 32767;
      val = 3*scale16(65535-val,m4_power>>2)+(m4_power>>2);
      val = scale16(val,val);
      TLC_setAll(val,val,val);

      break;
    }
    case 5:
    {
      // Guitar on fire mode
      
      dispVal += m1_dDisp;
      
      // calculate a "heat", as the center-of-maass point of the total power
      uint32_t heat = 0;
      for (int i=0; i<7; i++)
        heat += (65535/49)*i*eqVals[i];
      
      // make this be 0...65535 now
      heat = heat/curPower;
      heat = constrain(heat,m5_heatmin,m5_heatmax)-m5_heatmin;
      uint16_t h16 = (heat<<16)/(100+m5_heatmax-m5_heatmin);
      if (curPower < m5_pthresh)
        h16 = 0;
      
      // set hue based on distance and avg. power
      // interpolate and blah blah blah
      uint16_t pwr = avgPower;
      if (pwr > 8191) pwr = 8191;
      pwr = pwr << 3;
      pwr = scale16(pwr,h16);
      m5_heat = scale16(m5_heat,m5_decay) + scale16(pwr,65535-m5_decay);
      pwr = m5_heat;
      
      uint8_t pind = pwr >> 14; // 0...3, use ind
      uint16_t hue = lerp16by16(m5_hues[pind], m5_hues[pind+1], (pwr & ((1<<14)-1))<<2);
      // adjust intensity as well
      uint8_t brt = 0;
      brt = pwr >> 9 + 128;
  
      disperseHues(hue, dispVal, 0);
      huesToRGB(255, 128);
      
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
  delay(20);
}
