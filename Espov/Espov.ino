#include "Gyro.h"
#include "WS2812BI2S.h"
#include <Adafruit_NeoPixel.h>
#include "File.h"


static const int buttonPin = 5;
static const int ledpin = 27;
static const int ledCount = 45;
static const int speed = 3; 

//initialize a motion sensor with ascale = 0, gscale = 1
Gyro gyro(0, 1);


//image size is 128*128 and each pixel has 3 brightnesses, RGB
unsigned char image[128*128][3];

//image dimensions
int imageRes[] = {128, 128};

//starting from index 0 
int currentImage = 0;


bool loadCurrentImage()
{
  char filename[32];
  sprintf(filename, "/spiffs/image%d.bin", currentImage); //loads image to filename
  //if couldn't load image --> set zeros (default color)
  if(!readFromFile(filename, image[0], 128 * 128 * 3))
  {
    for(int y = 0; y < 128; y++)
      for(int x = 0; x < 128; x++)
      {
        image[y * 128 + x][0] = 0;//x * 2;
        image[y * 128 + x][1] = 0;//y * 2;
        image[y * 128 + x][2] = 0;//254 - x * 2;
      }    
    return false;
  } 
  return true;
}


void setup()
{
  Serial.begin(115200);
  initPixels();
  pinMode(buttonPin, INPUT);
  pinMode(ledpin,OUTPUT);
  
  gyro.calculateCorrection(); //calculate initial position 
  initFileSystem();
  loadCurrentImage();
}

int pressed = 0; //the button is pressed/not
bool on = false; // the strip is turned on/not


/////////////////////////
int visibleLeds = 0;


void turnOn()
{
  on = true;
  visibleLeds = 0;
  gyro.wakeUp();
}

void turnOff()
{
  on = false;
  visibleLeds = ledCount * speed + 100;
}



//update image according to current angle
void loopSaber(int dt)
{
  static float angle = 0;
  gyro.poll();

  //calculate distance from start position and see if strip moved ~more than 5
  float td = sqrt(gyro.rotationV[0] * gyro.rotationV[0] + gyro.rotationV[1] * gyro.rotationV[1] + gyro.rotationV[2] * gyro.rotationV[2]);
  float d = gyro.rotationV[2] * dt * 0.001;
  angle += d; 


   //standing still (correct angle)
  if(td < 5)
  {
    float l = sqrt(gyro.positionA[0] * gyro.positionA[0] + gyro.positionA[1] * gyro.positionA[1] + gyro.positionA[2] * gyro.positionA[2]);
    float rl = 1 / ((l == 0)? 1 : l);
    angle = angle * 0.9 + acos(rl * gyro.positionA[0]) * 180 / M_PI * 0.1;
  }

  //strip is moving --> calculate current angle
  float sx = -cos(angle * M_PI / 180);
  float sy = -sin(angle * M_PI / 180);
  int sample = 0;
  
  for(int i = 0; i < pixelCount; i++)
  {
    int x = 64 + (int)(sx * (i + 20));
    int y = 150 + (int)(sy * (i + 20));
    
    //update visible pixels 
    if(i * speed < visibleLeds)
    {
      int a = 0;
      if(x >= 0 && y >= 0 && x < imageRes[0] && y < imageRes[1])
        a = imageRes[0] * y + x;
      pixels[sample++] = bitLUT[((int)image[a][1] * image[a][1]) >> 8];
      pixels[sample++] = bitLUT[((int)image[a][0] * image[a][0]) >> 8];
      pixels[sample++] = bitLUT[((int)image[a][2] * image[a][2]) >> 8];
      res[i] = Strip.color(pixels[i],pixels[i+1],pixels[i+2]);
      
    }
    //update pixels out of the visible area to be a random color
    else
    {
      pixels[sample++] = bitLUT[0];
      pixels[sample++] = bitLUT[0];
      pixels[sample++] = bitLUT[0];  
      res[i] = Strip.color(pixels[i],pixels[i+1],pixels[i+2]);    
    }
    
  }
  //update the strip with the new pixels
  for(int i=0;i<ledCount;i++ ){
    strip.setPixelColor(i,res[i]);  
  }
  Strip.show();
 
}

void loop()
{
  static int time = 0;
  int t = millis();
  int dt = t - time;
  time = t;

  //if button is not pressed
  if(!digitalRead(buttonPin))
  {    
    pressed += dt;
    //if one second passed since the button was pressed --> one image has been displayed ---> turn off
    if(pressed > 1000)
    {
      if(on)
        turnOff();
      else
      {
        turnOn();
      }
      pressed = -10000; 
    }
  }
  //the button is pressed for more than 100 msc --> display next image
  else
  {
    if(pressed > 100)
    {
      currentImage = (currentImage + 1) & 3;
      loadCurrentImage();
    }    
    pressed = 0;
  } 
  loopSaber(dt); //update the next pixels
  if(on)
    visibleLeds += dt;
  else
    visibleLeds -= dt;

}