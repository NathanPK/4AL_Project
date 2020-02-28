#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#include<Wire.h>


#define ENABLE 6
#define DIRA 4
int forceHalfPeriod = 570;
const int MPU_addr=0x68;  // I2C address of the MPU-6050
long elapsedTime, AcX, AcY, AcZ; 

int yPosPin, yNegPin, xPosPin, xNegPin, intensityX, intensityY;
double scalarY = 255.0/16384;  //bits per 1g
double scalarX = 255.0/16384;  //bits per 1g

int trigPin = 3;    // Trigger
int echoPin = 2;    // Echo
long duration;
int forceUpFlag = 0;
unsigned long last_motor_drive;
unsigned long time_now;

void setup() {
  Serial.begin(115200);
  
  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ENABLE,OUTPUT);
  pinMode(DIRA,OUTPUT);
  digitalWrite(ENABLE,HIGH);
  last_motor_drive = millis();
  time_now = 0;
}
void loop() {  
    
  pinMode(echoPin, INPUT);
  duration = pulseIn(echoPin, HIGH);
  
  
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr,8,true);  // request a total of 14 registers
  AcX=Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)    
  AcY=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  AcZ=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  elapsedTime = millis();
  long dataArray[5] = {elapsedTime, AcX, AcY,AcZ,duration} ;

  for(int i = 0; i < (sizeof(dataArray) / sizeof(dataArray[0])-1); i++)
  {
    Serial.print(dataArray[i]);
    Serial.print(",");  
   }
   Serial.print(dataArray[4]);
   Serial.println();

   // Data is printed and in the dataArray
  time_now = millis();
  if((time_now - last_motor_drive) >= forceHalfPeriod) { // If we have surpassed forceHalfPeriod, switch driving motor state
    forceUpFlag = forceUpFlag ^ 1; // Switch the flag from 0 to 1 or vice versa using xor
    digitalWrite(DIRA, forceUpFlag);
    last_motor_drive = time_now; // Start counting time to next drive from now
  }
//    digitalWrite(DIRA, 1); //one way
//    delay(forceHalfPeriod); // Wait one half period before forcing up
//    digitalWrite(DIRA, 0);  //reverse
//    delay(forceHalfPeriod); // Wait one half period before forcing down

  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
}
