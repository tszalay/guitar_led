import hypermedia.net.*;

import processing.serial.*;

ColorPicker cp;
Serial myPort;

int[][] foo = new int[400][400];

UDP udpserver;

byte lastbyte = 0;

void setup() 
{
  size( 500, 500 );
  frameRate( 100 );
  
  cp = new ColorPicker( 10, 10, 400, 400, 255 );
  myPort = new Serial(this, "COM7", 115200);
  
  udpserver=new UDP(this, 8001);
  udpserver.listen(true);
}

void receive (byte[] data) {
  //.... process received data
  // and write to output
 // if (lastbyte != -56)
    myPort.write(data[0]);
  println(str(data[0]));
  lastbyte = data[0];
}

void draw ()
{
  background( 80 );
  cp.render();
}

public class ColorPicker 
{
  int x, y, w, h, c;
  PImage cpImage;
  
  public ColorPicker ( int x, int y, int w, int h, int c )
  {
    this.x = x;
    this.y = y;
    this.w = w;
    this.h = h;
    this.c = c;
    
    cpImage = new PImage( w, h );
    
    init();
  }
  
  private void init ()
  {
    // draw color.
    int cw = w - 60;
    for( int i=0; i<cw; i++ ) 
    {
      float nColorPercent = i / (float)cw;
      float rad = (-360 * nColorPercent) * (PI / 180);
      int nR = (int)(cos(rad) * 127 + 128) << 16;
      int nG = (int)(cos(rad + 2 * PI / 3) * 127 + 128) << 8;
      int nB = (int)(Math.cos(rad + 4 * PI / 3) * 127 + 128);
      int nColor = nR | nG | nB;
      
      setGradient( i, 0, 1, h/2, 0xFFFFFF, nColor );
      setGradient( i, (h/2), 1, h/2, nColor, 0x000000 );
    }
    
    // draw black/white.
    drawRect( cw, 0,   30, h/2, 0xFFFFFF );
    drawRect( cw, h/2, 30, h/2, 0 );
    
    // draw grey scale.
    for( int j=0; j<h; j++ )
    {
      int g = 255 - (int)(j/(float)(h-1) * 255 );
      drawRect( w-30, j, 30, 1, color( g, g, g ) );
    }
  }

  private void setGradient(int x, int y, float w, float h, int c1, int c2 )
  {
    float deltaR = red(c2) - red(c1);
    float deltaG = green(c2) - green(c1);
    float deltaB = blue(c2) - blue(c1);

    for (int j = y; j<(y+h); j++)
    {
      int c = color( red(c1)+(j-y)*(deltaR/h), green(c1)+(j-y)*(deltaG/h), blue(c1)+(j-y)*(deltaB/h) );
      cpImage.set( x, j, c );
      foo[x][j] = c;
    }
  }
  
  private void drawRect( int rx, int ry, int rw, int rh, int rc )
  {
    for(int i=rx; i<rx+rw; i++) 
    {
      for(int j=ry; j<ry+rh; j++) 
      {
        cpImage.set( i, j, rc );
      }
    }
  }
  
  public void render ()
  {
    image( cpImage, x, y );
    if( mousePressed &&
  mouseX >= x && 
  mouseX < x + w &&
  mouseY >= y &&
  mouseY < y + h )
    {
      //c = get( mouseX, mouseY );
      c = foo[mouseX-10][mouseY-10];
      // and write to output
      byte b = 0;
      b = (byte)((c>>16)&255);
      myPort.write(b);
      b = (byte)((c>>8)&255);
      myPort.write(b);
      b = (byte)(c&255);
      myPort.write(b);
    }
    fill( c );
    rect( x, y+h+10, 20, 20 );
  }
}
