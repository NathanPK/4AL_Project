/*
* Arduino Wireless Communication Reciever Accelerometer Data
* Library: TMRh20/RF24, https://github.com/tmrh20/RF24/
*/

#define ENABLE 5
#define DIRA 3
int forceHalfPeriod = 100;

long data[5];

long maxAmp;
long maxUltra;
long minUltra;
long maxFreq;
long currentStep;
int currentFreq;

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
RF24 radio(7, 8); // CE, CSN

const byte address[6] = "00002";
const long stepTime = 10000;

unsigned long time_now = 0;

void setup() {
  Serial.begin(115200);
  radio.begin();
  radio.setChannel(115);
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_2MBPS);
  radio.startListening();
  Serial.println("starting");

  // Initialize variables
  maxAmp = 0;
  maxUltra = 0;
  maxFreq = 0;
  currentFreq = 100;
  currentStep = 100;
  pinMode(ENABLE,OUTPUT);
  pinMode(DIRA,OUTPUT);
  digitalWrite(ENABLE,HIGH); // enable on
}

bool forceUpFlag = false;

bool runStep(long frequency) {
  unsigned long current_time = millis();
  if(forceUpFlag && (current_time-time_now) == forceHalfPeriod) {
    digitalWrite(DIRA, 0);
    time_now = millis();
  } else if(!forceUpFlag && (current_time-time_now) == forceHalfPeriod) {
    digitalWrite(DIRA, 1);
    time_now = millis();
  }
  
  // Initialize minUltra to first data point
  if(radio.available(0)) {
    radio.read(&data, sizeof(data));
    minUltra = data[4];
  }

  long firstTime = data[0];
  Serial.print("Data, endTime:");
  Serial.print(data[0]);
  Serial.println();
  Serial.print(data[0]-firstTime);
  Serial.println();
  
  while((data[0]-firstTime) < stepTime) { // Run step for certain amount of time
    if(radio.available(0)) {
      radio.read(&data, sizeof(data));
      for(int i = 0; i < (sizeof(data) / sizeof(data[0])-1); i++)
      {
        Serial.print(data[i]);
        Serial.print(",");
      }
      Serial.print(data[4]);
      Serial.println();
      maxUltra = data[4] > maxUltra ? data[4]:maxUltra;
      minUltra = data[4] < minUltra ? data[4]:minUltra;
    }
  }
  Serial.print("TIME");
  Serial.println(); Serial.print(data[0]-firstTime); Serial.println();

  Serial.print("Current Frequency: ");
  Serial.print(frequency);
  Serial.println();

  long amp = maxUltra-minUltra;
  Serial.print("Current Amplitude Found: ");
  Serial.print(amp); Serial.println();

  long oldAmp = maxAmp;
  maxAmp = amp > maxAmp ? amp:maxAmp;
  Serial.print("Current Maximum Amplitude: ");
  Serial.print(maxAmp); Serial.println();
  
  maxFreq = amp > maxAmp ? frequency:maxFreq;
  Serial.print("Current Maximum Frequency: ");
  Serial.print(maxFreq); Serial.println();

  if(amp < oldAmp) {
    return true;
  }
  return false;
}
 
void loop() {
  //print current step
  if(currentStep > 5) {
    time_now = millis();
    forceHalfPeriod = currentFreq;
    if(runStep(currentFreq)) {
      currentFreq -= currentStep;
      currentStep /= 2;
    } else {
      currentFreq += currentStep;
    }
  } else {
    Serial.print("MAXIMUM AMPLITUDE FOUND: ");
    Serial.print(maxAmp);
    Serial.println();
    Serial.print("RESONANCE FREQUENCY FOUND: ");
    Serial.print(maxFreq);
    Serial.println(); Serial.println();
    digitalWrite(DIRA, 1); //one way
    delay(forceHalfPeriod); // Wait one half period before forcing up
    digitalWrite(DIRA, 0);  //reverse
    delay(forceHalfPeriod); // Wait one half period before forcing down
  }
}
