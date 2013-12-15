// takes HSV and converts to RGB values
// input ranges are h = [0,359], s = [0,255], v = [0,255]
// and outputs are 16-bit unsigned RGB
// note: outputs are scaled for gamma of 2!

void HSVtoRGB(uint16_t h, uint16_t s, uint16_t v, uint16_t* rgb)
{
  uint16_t hh = h / 60;
  int16_t h12 = hh/2; //h over 120
  
  int16_t c = (v * s) / 256;
  int16_t x = (c * (60 - abs(((int)h-120*h12) - 60))) / 60;
  int16_t m = v - c; // 44
  
  switch(hh){
    case 0:
      rgb[0] = c + m;
      rgb[1] = x + m;
      rgb[2] = m;
      break;
    case 1:
      rgb[0] = x + m;
      rgb[1] = c + m;
      rgb[2] = m;
      break;
    case 2:
      rgb[0] = m;
      rgb[1] = c + m;
      rgb[2] = x + m;
      break;
    case 3:
      rgb[0] = m;
      rgb[1] = x + m;
      rgb[2] = c + m;
      break;
    case 4:
      rgb[0] = x + m;
      rgb[1] = m;
      rgb[2] = c + m;
      break;
    case 5:
      rgb[0] = c + m;
      rgb[1] = m;
      rgb[2] = x + m;
      break;
    default:
      rgb[0]=rgb[1]=rgb[2]=m;
  }
  // make sure we have a byte at this step
  // so that when we square it in the next one we don't overflow
  rgb[0] = constrain(rgb[0],0,255);
  rgb[1] = constrain(rgb[1],0,255);
  rgb[2] = constrain(rgb[2],0,255);
  // scale outputs for a gamma of 2
  rgb[0] *= rgb[0];
  rgb[1] *= rgb[1];
  rgb[2] *= rgb[2];  
}
