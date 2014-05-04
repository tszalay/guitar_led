// update period, in ms
const uint16_t loopTime = 20;
// and frequency
const uint16_t loopFreq = 1000/loopTime;

// here be parameters/magic numbers (for corresponding mode):
const uint16_t m1_slowTime = 100;        // time to loop hue, in seconds
const uint16_t m1_dispMag  = 7000;       // magnitude of dispersion
const uint8_t  m1_brt      = 50;         // brightness of the mode
const uint16_t m1_dHue = (65535/m1_slowTime/loopFreq); // amount to add each iteration, is about 21
const uint16_t m1_dDisp    = 5*m1_dHue;  // rate at which dispersion changes

const uint16_t m2_pwrScale  = 800;       // scaling for power
const uint8_t  m2_dispScale = 3;         // how much faster dispersion changes than power

const uint16_t m3_pwrScale = 1500;       // scaling for power to fade btwn LEDs
const uint16_t m3_baseSpd  = 300;        // amount to add no matter what
uint8_t        m3_curTgt   = 0;          // which LED is fading
uint8_t        m3_nextTgt  = 10;         // which LED is brightening
uint16_t       m3_curHue   = 0;          // hue of current LED
uint16_t       m3_nextHue  = 20000;      // hue of next LED
uint16_t       m3_fadeFac  = -1;         // fraction faded to next LED

uint16_t       m4_bright   = 0;          // current brightness of flashing
uint16_t       m4_wub      = 0;          // low-frequency pulsing
uint16_t       m4_wubstep  = 800;        // step increment for above
uint8_t        m4_state    = 0;          // state n stuff. 0=dark/fade, 1=wub
const uint16_t m4_decay    = 58000;      // rate at which it decays
const uint16_t m4_ton      = 30000;      // turn-on threshold
const uint16_t m4_toff     = 2000;       // quick decay/turn-off threshold

const uint16_t m5_hues[]   = {0, 5000, 10000, 15000, 20000};
const uint16_t m5_heatmin  = 17000;
const uint16_t m5_heatmax  = 37000;
uint16_t       m5_heat     = 0;
const uint16_t m5_decay    = 65000;
const uint16_t m5_pthresh  = 2000;
