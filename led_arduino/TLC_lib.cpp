// BOF preprocessor bug prevent - insert me on top of your arduino-code
// From: http://www.a-control.de/arduino-fehler/?lang=en
#if 1
__asm volatile ("nop");
#endif

#include "Arduino.h"
#include "TLC_lib.h"

// set DSPRPT to 1
// BLANK to 0
// EXTCLK to 0
// TMGRST 0 seems to be better
// OUTTMG doesn't matter
// bit order:
// 25h OUTTMG EXTGCK TMGRST DSPRPT BLANK BCBx7 BCGx7 BCRx7 data16x12
// also set red's BC bits to put out 40 mA instead of 50
// which is 100 in binary = 1100100
// 21 high bits for last brightness control
// write is 25h = 100101
// stop clock for 8x last clock period

// standard header as defined above
byte TLC_header[4] = 
               {  B10010100,  
                  B01011111, 
                  B11111111, 
                  B11100111 };

// definitions for data, clock pins
// and their port
#define TLC_DAT   2
#define TLC_CLK   4
#define TLC_PORT  PORTD

// how many devices do we have hooked up?
#define TLC_NDEV  6

// contents of message we are sending
byte TLC_msg[28*TLC_NDEV];

// indices into the above for each LED
uint16_t* LEDptr[16];

// this is the number of bits to right-shift all brightness values
// eg. to divide by 8, set to 3
byte TLC_dim = 0;



// utitility function funnction for swapping the byte order of a var or an array i'm tired ok?
__attribute__((always_inline)) inline static uint16_t flipBytes(uint16_t val)
{
  return ((uint16_t)lowByte(val) << 8) | highByte(val);
}

// and the same thing for in-place address, should be faster or something
__attribute__((always_inline)) inline static void flipBytes(uint16_t* ptr)
{
  uint8_t *p8 = (uint8_t*)ptr;
  *ptr = p8[0]*256U + p8[1];
}



void TLC_init(byte dimming) 
{
  pinMode(TLC_DAT, OUTPUT); // SDI
  pinMode(TLC_CLK, OUTPUT); // SCK
  
  TLC_dim = dimming;
  
  // set it all to 0 to start
  for (int i=0; i<TLC_NDEV*28; i++)
    TLC_msg[i] = 0;
  
  // set the first few bytes of each message block
  // to the required data header
  for (int i=0; i<TLC_NDEV; i++)
  {
    for (int j=0; j<4; j++)
      TLC_msg[i*28 + j] = TLC_header[j];
  }
  
  // and set the LED indices
  uint16_t *msg16 = (uint16_t*)TLC_msg;
  
  // we are indexing as follows: center boards, kidney boards, bottom boards
  for (int i=0; i<4; i++)
  {
    LEDptr[i] = msg16 + 0*14 + 3*i + 2;
    LEDptr[i+4] = msg16 + 3*14 + 3*i + 2;
  }
  // kidney, 8 LEDs down
  for (int i=0; i<2; i++)
  {
    LEDptr[i+8] = msg16 + 1*14 + 3*i + 2 + 6;
    LEDptr[i+10] = msg16 + 2*14 + 3*i + 2 + 6;
  }
  
  // bottom, 12 LEDs down
  for (int i=0; i<2; i++)
  {
    LEDptr[i+12] = msg16 + 4*14 + 3*i + 2 + 3;
    LEDptr[i+14] = msg16 + 5*14 + 3*i + 2 + 3;
  }
}

void TLC_setAll(uint16_t R, uint16_t G, uint16_t B)
{
  // flip endianness of R, G, B cuz arduino
  R = flipBytes(R >> TLC_dim);
  G = flipBytes(G >> TLC_dim);
  B = flipBytes(B >> TLC_dim);  
  
  // use lookup table to set everything
  for (int i=0; i<16; i++)
  {
    LEDptr[i][2] = R;
    LEDptr[i][1] = G;
    LEDptr[i][0] = B;
  }
}

// turn a single LED color on, indexed by 48 LEDs
void TLC_singleLED(int ledIndex)
{
  uint16_t val = 65535;
  
  for (int i=0; i<16; i++)
    for (int j=0; j<3; j++)
      LEDptr[i][j] = 0;
      
  if (ledIndex >= 48) return;
  
  LEDptr[ledIndex/3][2-(ledIndex%3)] = val;
}

// takes array of 48 values, RGBRGBRGBetc
void TLC_setData(uint16_t *data)
{
  // use lookup table to set everything
  for (int i=0; i<16; i++)
  {
    LEDptr[i][2] = flipBytes(*data++ >> TLC_dim);
    LEDptr[i][1] = flipBytes(*data++ >> TLC_dim);;
    LEDptr[i][0] = flipBytes(*data++ >> TLC_dim);;
  }
}

// a templated function for writing a single bit to the output, in a given position
template <uint8_t BIT> inline __attribute__((always_inline)) static void TLC_writeBit(register uint8_t b, register uint8_t ll, register uint8_t lh, register uint8_t hl, register uint8_t hh) 
{ 
  if(b & (1 << BIT))
  {
    // write clock lo-hi with data hi
    TLC_PORT = hl;
    TLC_PORT = hl;
    TLC_PORT = hh;
    TLC_PORT = hh;
    TLC_PORT = hl;
  } 
  else
  {
    TLC_PORT = ll;
    TLC_PORT = ll;
    TLC_PORT = ll;
    
    TLC_PORT = lh;
    TLC_PORT = lh;
    TLC_PORT = ll;
  }
}

void TLC_write()
{
  // this line turns out to be quite critical for precision timing
  // and preventing random latching and all sorts of other bugs
  noInterrupts();
  
  // pre-computed, cached port values for speed
  register uint8_t ll = PORTD & ~(_BV(TLC_DAT)|_BV(TLC_CLK));
  register uint8_t lh = (PORTD & ~(_BV(TLC_DAT)))|_BV(TLC_CLK);
  register uint8_t hl = (PORTD & ~(_BV(TLC_CLK)))|_BV(TLC_DAT);
  register uint8_t hh = PORTD | _BV(TLC_DAT) | _BV(TLC_CLK);
  
  // write message byte-by-byte
  uint8_t* msgend = TLC_msg+28*TLC_NDEV;
  uint8_t *msg = TLC_msg;
  while (msg < msgend)
  {
    uint8_t b = *msg++;
    TLC_writeBit<7>(b,ll,lh,hl,hh);
    TLC_writeBit<6>(b,ll,lh,hl,hh);
    TLC_writeBit<5>(b,ll,lh,hl,hh);
    TLC_writeBit<4>(b,ll,lh,hl,hh);
    TLC_writeBit<3>(b,ll,lh,hl,hh);
    TLC_writeBit<2>(b,ll,lh,hl,hh);
    TLC_writeBit<1>(b,ll,lh,hl,hh);
    TLC_writeBit<0>(b,ll,lh,hl,hh);
  }
    
  // end with clock low and data high
  TLC_PORT = hl;
  
  interrupts();
}

