import processing.serial.*;

ColorPicker cp;
Serial myPort;

int cpSize = 400;
int lf = 10;    // Linefeed in ASCII

// sensor data read from the arduino
// order: 7x EQ, 1x selector, 3x accel, 1x distance
int nSensors = 13;
float sensorVals[] = new float[nSensors];

// buffered sensor values
int nBuffered = 5;
int bufLength = 500;
float sensorBuffer[][] = new float[nBuffered][bufLength];

void setup() 
{
  size( 700, 450 );
  frameRate( 100 );
  
  cp = new ColorPicker( 10, 10, cpSize, cpSize, color(128) );
  
  try {
    // attempt to connect, and sync input
    myPort = new Serial(this, "COM4", 115200);
    myPort.readBytesUntil(lf);
  }
  catch (Exception e) {
    println("No Arduino detected, running without Serial.");
    for (int i=0; i<7; i++)
      sensorVals[i] = random(4096);
  }
}

void draw ()
{
  background( 80 );
  cp.render();
  drawSensors();
  
  readSensors();
}

void drawSensors()
{
  int xstart = cpSize + 30;
  int ystart = 10;
  int eqHeight = 80;
  int eqSpc = 40;
  int eqWid = 20;
  
  for (int i=0; i<7; i++)
  {
    // draw bright boxes first, then mask with dark one
    // sneaky!
    noStroke();
    fill(color(0,255,0));
    rect( xstart+eqSpc*i, ystart, eqWid, eqHeight );
    
    fill(color(240,240,0));
    rect( xstart+eqSpc*i, ystart, eqWid, eqHeight/2.5 );
    
    fill(color(200,0,0));
    rect( xstart+eqSpc*i, ystart, eqWid, eqHeight/7 );
    
    fill(color(0,60,0));
    rect( xstart+eqSpc*i, ystart, eqWid, (65535.0-sensorVals[i])*(eqHeight/65535.0) );
    
    stroke(color(0));
    noFill();
    rect( xstart+eqSpc*i, ystart, eqWid, eqHeight );
  }
  
  // now draw accelerometer data
  ystart += eqHeight + 20;
  int w = 7*eqSpc-eqWid;
  
  // first draw the background box
  drawStreamBox(xstart,ystart,w,eqHeight,15);
  // and then the streams
  drawStream(sensorBuffer[0],xstart,ystart,w,eqHeight,color(255,0,0),true);
  drawStream(sensorBuffer[1],xstart,ystart,w,eqHeight,color(0,255,0),true);
  drawStream(sensorBuffer[2],xstart,ystart,w,eqHeight,color(0,0,255),true);
  
  // shift again
  ystart += eqHeight + 20;
  
  // now draw distance sensor stream
  drawStreamBox(xstart,ystart,w,eqHeight,15);
  drawStream(sensorBuffer[3],xstart,ystart,w,eqHeight,color(255,255,0),false);
  // and any extra sensors
  drawStream(sensorBuffer[4],xstart,ystart,w,eqHeight,color(0,255,255),false);
  
  
  // and finally draw the selector switch
  ystart += eqHeight + 20 + 20;
  int xc = xstart + w/2;
  w = 50;
  int yc = ystart + w/2;  
  for (int i=0; i<6; i++)
  {
    fill(0,60,0);
    stroke(0);
    
    if (i==(int)sensorVals[7])
      fill(0,180,0);
      
    triangle(xc+w*cos((i-2)*PI/3),yc+w*sin((i-2)*PI/3),xc+w*cos((i-1)*PI/3),yc+w*sin((i-1)*PI/3),xc,yc);
  }
}

void drawStream(float data[], int x, int y, int w, int h, int c, boolean signed)
{
  int n = data.length;
  
  stroke(c);
  
  for (int i=0; i<n-1; i++)
  {
    float x0 = map(i, 0, n-1, x, x+w);
    float x1 = map(i+1, 0, n-1, x, x+w);
    if (signed)
    {
      float y0 = map(data[i],-32768,32767,y+h,y);
      float y1 = map(data[i+1],-32768,32767,y+h,y);
      line(x0,y0,x1,y1);
    }
    else
    {
      float y0 = map(data[i],0,65535,y+h,y);
      float y1 = map(data[i+1],0,65535,y+h,y);
      line(x0,y0,x1,y1);
    }
  }
}

// draw a data stream rendering box
void drawStreamBox(int x, int y, int w, int h, int s)
{
  // da box
  fill(0);
  stroke(0);
  rect(x,y,w,h);
  
  // da lines
  stroke(60);
  for (int yl=h/2; yl>0; yl-=s)
  {
    line(x,yl+y,x+w,yl+y);
    line(x,y+(h-yl),x+w,y+(h-yl));
  }
  for (int xl=w/2; xl>0; xl-=s)
  {
    line(x+xl,y,x+xl,y+h);
    line(x+(w-xl),y,x+(w-xl),y+h);
  }
}

void readSensors()
{
  if (myPort != null)
  {
    while (myPort.available() > 0) {
      byte buf[] = myPort.readBytesUntil(lf);
      if (buf == null)
        break;
      String myString = new String(buf);
      print(myString);
      // split and parse, save in our array
      if (myString != null)
      {
        //print(myString);
        String parts[] = myString.split(",");
        for (int i=0; i<min(parts.length,nSensors); i++)
          sensorVals[i] = float(parts[i]);
      }
    }
  }
  else
  {
    // make up some phony numbers
    for (int i=0; i<nSensors; i++)
    {
      if (i != 7)
        sensorVals[i] += (random(21)-10);
    }
    
    for (int i=0; i<7; i++)
    {
      if (sensorVals[i] < 0) sensorVals[i] = 0;
      if (sensorVals[i] > 4095) sensorVals[i] = 4096;
    }
    
    if (random(1) < 0.01)
      sensorVals[7] = floor(random(6));
    
    for (int i=8; i<nSensors; i++)
    {
      if (sensorVals[i] < -2048) sensorVals[i] = -2048;
      if (sensorVals[i] > 2047) sensorVals[i] = 2047;
    }
  }
  
  // and cycle buffers
  for (int i=0; i<nBuffered; i++)
  {
    arrayCopy(sensorBuffer[i], 1, sensorBuffer[i], 0, bufLength-1);
    sensorBuffer[i][bufLength-1] = sensorVals[i+8];  
  }
}

public class ColorPicker 
{
  int x0, y0, w, h, c;
  PImage cpImage;
  
  public ColorPicker ( int x, int y, int w, int h, int c )
  {
    this.x0 = x;
    this.y0 = y;
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
    
    colorMode(HSB, cw, w, w);
    for (int x=0; x<cw; x++)
    {
      for (int y=0; y<h; y++)
      {
        if (y < h/2) // draw saturation box up top
          cpImage.set( x, y, color(x, y*2, w));
        else // and brightness (darkness?) box down below
          cpImage.set( x, y, color(x, w, w - (y-h/2)*2));
      }
    }
    colorMode(RGB, 255);
    
    // draw black/white.
    drawRect( cw, 0,   30, h/2, color(255) );
    drawRect( cw, h/2, 30, h/2, color(0) );
    
    // draw grey scale.
    for( int j=0; j<h; j++ )
    {
      int g = 255 - (int)(j/(float)(h-1) * 255 );
      drawRect( w-30, j, 30, 1, color( g, g, g ) );
    }
  }
  
  private void drawRect( int rx, int ry, int rw, int rh, int rc )
  {
    for(int i=rx; i<rx+rw; i++) 
      for(int j=ry; j<ry+rh; j++) 
        cpImage.set( i, j, rc );
  }
  
  public void render ()
  {
    image( cpImage, x0, y0 );
    // get mouse position relative to picker
    int mX = mouseX-x0;
    int mY = mouseY-y0;
    
    if( mousePressed &&
        mX >= 0 && mX < w &&
        mY >= 0 && mY < h)
    {
      c = cpImage.get(mX, mY);
      // and write to output
      byte b = 0;
      b = (byte)((c>>16)&255);
      //myPort.write(b);
      b = (byte)((c>>8)&255);
      //myPort.write(b);
      b = (byte)(c&255);
      //myPort.write(b);
    }
    // draw the colored bottom rectangle
    fill( c );
    stroke(0);
    rect( x0, y0+h+10, 20, 20 );
  }
}
