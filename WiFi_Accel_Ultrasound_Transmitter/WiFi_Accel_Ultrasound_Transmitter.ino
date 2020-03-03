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
float duration;
int forceUpFlag = 0;
float sound_speed= 343000;

unsigned long last_motor_drive;

unsigned long step_duration = 10000;
unsigned long data_duration = 2000;
unsigned long time_now;
unsigned long start_time;
long max_amplitude;
long curr_max;
int overstep_counter = 0;
int step_direction = 1;
int step_size = 40;

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
  curr_max = 0;
}

void stabilize(){
  start_time = millis();
  while (millis() - start_time < step_duration){
    digitalWrite(DIRA, 1); //one way
    delay(forceHalfPeriod); // Wait one half period before forcing up
    digitalWrite(DIRA, 0);  //reverse
    delay(forceHalfPeriod); // Wait one half period before forcing down
  }
}

long collect_data(){
  start_time = millis();
  long maxUltra = 0;
  long minUltra = -1;
  while (millis() - start_time < data_duration){
    pinMode(echoPin, INPUT);
    duration = pulseIn(echoPin, HIGH)*sound_speed/2000000;
    
    Wire.beginTransmission(MPU_addr);
    Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_addr,8,true);  // request a total of 14 registers
    AcX=Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)    
    AcY=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
    AcZ=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
    elapsedTime = millis();
    long dataArray[5] = {elapsedTime, AcX, AcY,AcZ,duration};

    for(int i = 0; i < (sizeof(dataArray) / sizeof(dataArray[0])-1); i++)
    {
      Serial.print(dataArray[i]);
      Serial.print(",");  
     }
     Serial.print(dataArray[4]);
     Serial.println();

    digitalWrite(trigPin, LOW);
    delayMicroseconds(5);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

   // Data is printed and in the dataArray
    // Initialize minUltra if not yet initialized
    if (minUltra == -1)
      minUltra = dataArray[4];
      
     // Check for Max and Min
     maxUltra = dataArray[4] > maxUltra ? dataArray[4]:maxUltra;
     minUltra = dataArray[4] < minUltra ? dataArray[4]:minUltra;
    time_now = millis();
    if((time_now - last_motor_drive) >= forceHalfPeriod) { // If we have surpassed forceHalfPeriod, switch driving motor state
      forceUpFlag = forceUpFlag ^ 1; // Switch the flag from 0 to 1 or vice versa using xor
      digitalWrite(DIRA, forceUpFlag);
      last_motor_drive = time_now; // Start counting time to next drive from now
    }
  }
  return (maxUltra-minUltra)/2;
}

void run_infinite(){

  pinMode(echoPin, INPUT);
  duration = pulseIn(echoPin, HIGH)*sound_speed/2000000;
  
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr,8,true);  // request a total of 14 registers
  AcX=Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)    
  AcY=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  AcZ=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  elapsedTime = millis();
  long dataArray[5] = {elapsedTime, AcX, AcY,AcZ,duration};

  for(int i = 0; i < (sizeof(dataArray) / sizeof(dataArray[0])-1); i++)
  {
    Serial.print(dataArray[i]);
    Serial.print(",");  
   }
   Serial.print(dataArray[4]);
   Serial.print(",");
   Serial.print(step_size);
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

/*
 * Run for 10 seconds
 * Run for 2 seconds, determine curr_amplitude, collect data
 * if curr_amplitude >= MAX:
 *  Replace MAX with curr_amplitude
 *  step in Change direction
 * else if
 *  curr_amplitude < MAX:
 *  add 1 to overstep_counter
 *  if overstep_counter == 2
 *    reset overstep_counter
 *    reset MAX
 *    flip Change direction
 *    step
 *   
 */
void loop() {  
  if (step_size < 5){
    forceHalfPeriod = forceHalfPeriod  + 2 * (step_direction * step_size);
    while(1){
      run_infinite();
    }
  }
  else{
    stabilize();
    curr_max = collect_data();
    if ( curr_max > max_amplitude){
      max_amplitude = curr_max;
      overstep_counter = 0;
      forceHalfPeriod = forceHalfPeriod + (step_direction * step_size);
    }
    else if (curr_max < max_amplitude){
      overstep_counter++;
      if (overstep_counter == 2){
        overstep_counter = 0;
        max_amplitude = 0;
        step_direction = step_direction * (-1);
        step_size = step_size / 2;
        forceHalfPeriod = forceHalfPeriod + (step_direction * step_size);
      }
    }
  }
    
//  pinMode(echoPin, INPUT);
//  long sample_amplitude = pulseIn(echoPin, HIGH);
//  
//  
//  
  pinMode(echoPin, INPUT);
  duration = pulseIn(echoPin, HIGH)*sound_speed/2000000;
  
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr,8,true);  // request a total of 14 registers
  AcX=Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)    
  AcY=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  AcZ=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  elapsedTime = millis();
  long dataArray[5] = {elapsedTime, AcX, AcY,AcZ,duration};

  for(int i = 0; i < (sizeof(dataArray) / sizeof(dataArray[0])-1); i++)
  {
    Serial.print(dataArray[i]);
    Serial.print(",");  
   }
   Serial.print(dataArray[4]);
   Serial.println();

     digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

   // Data is printed and in the dataArray
  time_now = millis();
  if((time_now - last_motor_drive) >= forceHalfPeriod) { // If we have surpassed forceHalfPeriod, switch driving motor state
    forceUpFlag = forceUpFlag ^ 1; // Switch the flag from 0 to 1 or vice versa using xor
    digitalWrite(DIRA, forceUpFlag);
    last_motor_drive = time_now; // Start counting time to next drive from now
  }
////    digitalWrite(DIRA, 1); //one way
////    delay(forceHalfPeriod); // Wait one half period before forcing up
////    digitalWrite(DIRA, 0);  //reverse
////    delay(forceHalfPeriod); // Wait one half period before forcing down
//

  
}
