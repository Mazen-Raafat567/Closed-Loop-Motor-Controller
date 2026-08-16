#include <Wire.h>
#include <LiquidCrystal_I2C.h>



//Pins
const byte encoderPin = 2;
const byte speedPin = 9;
const byte dir1Pin = 7; 
const byte dir2Pin = 8; 
const byte potPin = A0;
const byte dirBtn = 5;
const byte EStopBtn = 3;
const byte EStopLed = 4;


int potVal;
float filteredPot;
int motorPWM;

//PID control
float PIDOutput;
float error;
float errorIntegral = 0;

float kp = 0.2;
float ki = 0.8;

float maxIntegral = 255.0 / ki;


//Kick Start
bool kickStartActive = false;
bool motorRunning = false;
unsigned long kickStartTime = 0;
const unsigned long kickDuration = 100;
int dynamicKickStartPWM = 0;

//Rpm Variables
volatile unsigned long currentPulseTime = 0;
volatile unsigned long lastPulseTime = 0;
unsigned long timeSinceLastPulse;

const byte BUFFER_SIZE = 4;
volatile unsigned long periodBuffer[BUFFER_SIZE] = {0};
volatile byte bufferIndex = 0;
volatile unsigned long periodSum = 0;

float rpm;
float targetRpm;
float filteredRpm;
unsigned long lastPidTime = 0;
unsigned long currentPidTime;

//change Direction
bool currentBtnState;
bool oldBtnState = HIGH;
bool direction = false;
bool requestedDirection = false;
bool changingDirection = false;

//Emergency stop
volatile bool EStopActive = false;
unsigned long lastELedTime;
bool ELedState = false;

//throttle check
bool needThrottleReset;

//LCD
unsigned long lastTimeOfLCD = 0;
LiquidCrystal_I2C lcd(0x27, 16, 2);


void setup() {



Serial.begin(115200);

lcd.init();
lcd.backlight();

lcd.setCursor(0,0);
lcd.print("RPM:");
lcd.setCursor(0,1);
lcd.print("PWM:");

pinMode(encoderPin,INPUT);
pinMode(speedPin,OUTPUT);
pinMode(dir1Pin,OUTPUT);
pinMode(dir2Pin,OUTPUT);
pinMode(potPin,INPUT);
pinMode(dirBtn,INPUT_PULLUP);
pinMode(EStopBtn,INPUT_PULLUP);
pinMode(EStopLed,OUTPUT);

attachInterrupt(digitalPinToInterrupt(encoderPin),encoderISR,FALLING);
attachInterrupt(digitalPinToInterrupt(EStopBtn),EStopISR,FALLING);

EIFR = bit(INTF0) | bit(INTF1); //Eliminate intialization noise
EStopActive = false;
needThrottleReset = true; 

filteredPot = analogRead(potPin); //pre load the potentiometer value to enable needThrottleReset immediately

}

void loop() {

  readInputs();
  motorState();
  calculateRpm();
  run_50ms();
  driveMotor();
  LCD();







  //Tuning and debugging

/*
Serial.print("Period:");
Serial.print(periodSum/4);
Serial.print(",");

Serial.print("Rejected:");
Serial.print(rejectedPulses);
Serial.print(",");

Serial.print("Rejected_Period:");
Serial.print(rejectedPeriod);
Serial.print(",");

Serial.print("Raw_RPM:");
Serial.print(rpm);
Serial.print(",");

Serial.print("PWM:");
Serial.print(motorPWM);
Serial.print(",");

Serial.print("Target_RPM:");
Serial.print(targetRpm);
Serial.print(",");

Serial.print("Filtered_RPM:");
Serial.println(filteredRpm);
*/


}


void encoderISR() {
  currentPulseTime = micros();
  unsigned long tempPeriod = currentPulseTime - lastPulseTime;
  
  if (tempPeriod > 11000){  //eleminate impossible rpm spikes (>~270 rpm) caused by noisy encoder signals 
    lastPulseTime = currentPulseTime;

    periodSum -= periodBuffer[bufferIndex]; //remove old value
    periodBuffer[bufferIndex] = tempPeriod; //add new value 
    periodSum += tempPeriod;                // update the total

    bufferIndex = (bufferIndex + 1)%BUFFER_SIZE; // cycle the array index
  }
}


void EStopISR(){
  EStopActive = true;
  motorPWM=0;
  analogWrite(speedPin,motorPWM);
}

void ELedBlink(){

if(millis() - lastELedTime >=200){
  ELedState =!ELedState;
  digitalWrite(EStopLed,ELedState);
  lastELedTime = millis();
}

}


void readInputs(){
  //Read direction button
  static unsigned long lastDirBtnTime = 0;
  currentBtnState = digitalRead(dirBtn);
  if(oldBtnState == HIGH && currentBtnState == LOW){
    if(millis() - lastDirBtnTime >=50){
      requestedDirection=!requestedDirection;
      lastDirBtnTime = millis();
    }
  }
  oldBtnState = currentBtnState;

  //Read potentiometer
  potVal = analogRead(potPin);
  filteredPot = (0.9*filteredPot) + (0.1*potVal);
  if(filteredPot<50 && needThrottleReset == true){
    needThrottleReset = false;
  }

    
}


void motorState(){
  if (needThrottleReset == true || EStopActive == true){
    motorPWM = 0;
    targetRpm = 0;
    return;
  }
  
  //Check potentiometer value
  if(filteredPot < 100){   //Potentiometer deadzone
    motorPWM = 0;
    targetRpm = 0;
  }
  else{
    targetRpm = map(filteredPot,100,1023,0,250);
  }

  //Check direction change
  if(requestedDirection!=direction){
    changingDirection = true;
  }

  //Check kick start
  else if(targetRpm>0 && motorRunning == false && kickStartActive == false){
    kickStartActive = true;
    kickStartTime = millis();
    motorRunning = true;
    dynamicKickStartPWM = map(targetRpm, 0, 240, 100, 120);
  }
}


void driveMotor(){

  //Determine direction
  if(direction){                
    digitalWrite(dir1Pin,LOW);
    digitalWrite(dir2Pin,HIGH);
  }
  else{
    digitalWrite(dir1Pin,HIGH);
    digitalWrite(dir2Pin,LOW);
  }

  if(EStopActive == true){
    motorPWM=0;
    errorIntegral=0;
    analogWrite(speedPin,motorPWM);
    ELedBlink();
    return;
  }

  else if(needThrottleReset == true){
    motorPWM=0;
    errorIntegral=0;
    analogWrite(speedPin,motorPWM);
    return;
  }

  //Direction change state
  else if(changingDirection == true){
    motorPWM = 0;
    
    if(filteredRpm<20){
      direction = requestedDirection;
      changingDirection = false;
      errorIntegral = 0;
      motorRunning = false;
    }
  }

  //kick start state
  else if (kickStartActive){
    motorPWM = dynamicKickStartPWM;
    if (millis() - kickStartTime >= kickDuration){  //end kickstart
      kickStartActive = false;

    if (ki > 0) {
        float currentError = targetRpm - filteredRpm;
        errorIntegral = (dynamicKickStartPWM - (kp * currentError)) / ki;
      }
    }
  }


  else if (targetRpm == 0){
    motorPWM = 0;
    errorIntegral = 0;
  }
  
  analogWrite(speedPin,motorPWM);
}
  



void calculateRpm() {
  noInterrupts();
  unsigned long currentPeriodSum = periodSum;
  timeSinceLastPulse = micros() - lastPulseTime;
  interrupts();

  if (timeSinceLastPulse > 200000) {
    rpm = 0;
  } 
  else if (currentPeriodSum > 0) {
    // RPM = (Pulses Measured / Total Pulses) * (Microseconds in a Minute / Microseconds Measured)
    
    rpm = 12000000.0 / currentPeriodSum;
  }
}

void run_50ms(){

    currentPidTime = millis();

  if (currentPidTime - lastPidTime >= 50) {
    float dt = (currentPidTime - lastPidTime) / 1000.0;
    lastPidTime = currentPidTime;
    
    filteredRpm = (0.6*filteredRpm) + (0.4*rpm); 
    
    if (targetRpm > 0 && EStopActive == false) {
      PIDControl(dt);
    }
  
      //for simulink
      float telemetry[5];

      telemetry[0] = (float)(periodSum / 4); 
      telemetry[1] = (float)motorPWM;
      telemetry[2] = targetRpm;
      telemetry[3] = filteredRpm;
      telemetry[4] = error;
  
      uint8_t header[2] = {0xAA, 0xBB}; 
      Serial.write(header, 2);
  
      Serial.write((uint8_t*)telemetry, sizeof(telemetry));
  }
  
  if (filteredRpm < 10 && targetRpm == 0) {
    motorRunning = false;
  }

}

void PIDControl(float dt){

  error = targetRpm - filteredRpm;

  if (abs(error)<3) { //Eliminate minor error values caused by the hardware limitations
    error = 0; 
  }

  errorIntegral = errorIntegral + error*dt;
  errorIntegral = constrain(errorIntegral,-maxIntegral,maxIntegral);
  PIDOutput = kp*error + ki*errorIntegral;

  if(motorRunning == false){
    errorIntegral=0;
  }

  motorPWM = constrain(PIDOutput,35,255);

}


void LCD(){

  

  if(millis()-lastTimeOfLCD >=250){
    lastTimeOfLCD = millis();
    if(EStopActive == true){
    lcd.setCursor(0,0);
    lcd.print("EMERGENCY STOP! ");

    lcd.setCursor(0,1);
    lcd.print("RESET REQUIRED  ");
    }
    else if(needThrottleReset == true){
      lcd.setCursor(0, 0);
      lcd.print("LOWER THROTTLE  ");
      lcd.setCursor(0, 1);
      lcd.print("TO ZERO FIRST   ");
    }
  
  else{
    lcd.setCursor(0,0);
    lcd.print("RPM:");
    lcd.setCursor(4,0);
    lcd.print("     ");
    lcd.setCursor(4,0);
    lcd.print(filteredRpm,0);

    lcd.setCursor(0,1);
    lcd.print("PWM:");
    lcd.setCursor(4,1);
    lcd.print("      ");
    lcd.setCursor(4,1);
    lcd.print(motorPWM);
  

    lcd.setCursor(9,0);
    lcd.print("       ");
    lcd.setCursor(9,1);
    lcd.print("       ");
    lcd.setCursor(9,0);
    if(direction){
        lcd.print("REV");
    }
    else{
        lcd.print("FWD");
    }
  }
  }
}
