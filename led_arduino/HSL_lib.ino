void HSV_to_RGB(float h, float s, float v, byte *r, byte *g, byte *b)
{
  int i,f,p,q,t;
  
  h = max(0.0, min(360.0, h));
  s = max(0.0, min(100.0, s));
  v = max(0.0, min(100.0, v));
  
  s /= 100;
  v /= 100;
  
  if(s == 0) {
    // Achromatic (grey)
    *r = *g = *b = round(v*255);
    return;
  }
 
  h /= 60; // sector 0 to 5
  i = floor(h);
  f = h - i; // factorial part of h
  p = v * (1 - s);
  q = v * (1 - s * f);
  t = v * (1 - s * (1 - f));
  switch(i) {
    case 0:
      *r = round(255*v);
      *g = round(255*t);
      *b = round(255*p);
      break;
    case 1:
      *r = round(255*q);
      *g = round(255*v);
      *b = round(255*p);
      break;
    case 2:
      *r = round(255*p);
      *g = round(255*v);
      *b = round(255*t);
      break;
    case 3:
      *r = round(255*p);
      *g = round(255*q);
      *b = round(255*v);
      break;
    case 4:
      *r = round(255*t);
      *g = round(255*p);
      *b = round(255*v);
      break;
    default: // case 5:
      *r = round(255*v);
      *g = round(255*p);
      *b = round(255*q);
    }
}

void HslToRgb( int hue, byte sat, byte light, byte* lpOutRgb )
{
  if( sat == 0 )
  {
    lpOutRgb[0] = lpOutRgb[1] = lpOutRgb[2] = light;
    return;
  }
  
  float nhue = (float)hue * (1.0f / 360.0f);
  float nsat = (float)sat * (1.0f / 255.0f);
  float nlight = (float)light * (1.0f / 255.0f);
  
  return HSV_to_RGB((float)hue, (float)sat, (float)light, lpOutRgb, lpOutRgb+1, lpOutRgb+2);
  
  
  float m2;
  if( light < 128 )
    m2 = nlight * ( 1.0f + nsat );
  else
    m2 = ( nlight + nsat ) - ( nsat * nlight );

  float m1 = ( 2.0f * nlight ) - m2;
  
  lpOutRgb[0] = HueToChannel( m1, m2, nhue + (1.0f / 3.0f) );
  lpOutRgb[1] = HueToChannel( m1, m2, nhue );
  lpOutRgb[2] = HueToChannel( m1, m2, nhue - (1.0f / 3.0f) );
}

byte HueToChannel( float m1, float m2, float h )
{
  if( h < 0.0f ) h += 1.0f;
  if( h > 1.0f ) h -= 1.0f;
  if( ( 6.0f * h ) < 1.0f )
    return (byte)( 255.0f * ( m1 + ( m2 - m1 ) * 6.0f * h ) );
  if( ( 2.0f * h ) < 1.0f )
    return (byte)( 255.0f * m2 );
  if( ( 3.0f * h ) < 2.0f ) 
    return (byte)( 255.0f * ( m1 + ( m2 - m1 ) * ((2.0f/3.0f) - h) * 6.0f ) );
    
  return (byte)( 255.0f * m1 );
}


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
  rgb[0] = constrain(rgb[0],0,255);
  rgb[1] = constrain(rgb[1],0,255);
  rgb[2] = constrain(rgb[2],0,255);
  rgb[0] *= rgb[0];
  rgb[1] *= rgb[1];
  rgb[2] *= rgb[2];  
}
