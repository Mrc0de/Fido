#include "Arduino.h"
#include "heltec.h"   // Designed for a Heltec Lora32 V2
                      // I avoid their library, I just want the parts, that might change.
                      // I'm not getting any license, I'll reverse engi one first
/////////////////                      
// ServoPWM Stuff
#include <Adafruit_PWMServoDriver.h>
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40, Wire1);
#define SERVOMIN  150 // This is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX  600 // This is the 'maximum' pulse length count (out of 4096)
#define OFFSETMIN 5 // Apply +OFFSETMIN to SERVOMIN calcs to prevent overrun (condition only, dont change startpoint)
#define OFFSETMAX 25 // Apply -OFFSETMAX to SERVOMAX calcs to prevent overrun (condition only, dont change startpoint)
#define SERVO_FREQ 50 // Analog servos run at ~50 Hz updates

///////
// PSUs
# define ServoPSUKeyPin 13
//# define MCUPSUKeyPin ?? /// TODO

///////////////////
// UPPER LEG SERVOS
#define FRONT_L 0
#define FRONT_R 1
#define REAR_L 2
#define REAR_R 3

/////////////
// Directions
#define CW false
#define CCW true

void resetFrontLeft(int rev=0);
void resetFrontRight(int rev=0);
void resetRearLeft(int rev=0);
void resetRearRight(int rev=0);
//////////
// Globals
//////////////
// PSU Tapping
int StdTapInterval = 10000;
bool TapServoPSU;
unsigned long LastTapServoPSU = 0;
bool TapMCUPSU;
unsigned long LastTapMCUPSU = 0;
bool demoFlip = false;



void setup() {  
  //////////////////////////////////////////////////////////////////////////
  // Enables OLED on I2C on pins: 4(OLED_SDA), 15(OLED_SCL) and 16(OLED_RST)
  Heltec.begin(true, false, true);
  // Heltec uses Wire for this, there are TWO Wire objects prepared, Wire and Wire1
  // To avoid our built in OLED entirely, we will cede Wire to it completely and just use Wire1 (easy compromise, no workarounds)
  Wire1.begin(SDA,SCL);  // SDA is 21 and SCL is 22 (notice OLED_SDA vs plain old SDA) ty heltec
  ///////////////////
  // Initial Feedback
  Heltec.display->clear();
  Heltec.display->drawString(0, 0, "[STARTING]");
  Heltec.display->display();
  delay(1500);
  //////////////
  // Final Pause
  Heltec.display->clear();
  Heltec.display->drawString(0, 0, "[INIT COMPLETE]");
  Heltec.display->drawString(0, 8, "[RESET WILL BEGIN]");
  Heltec.display->drawString(0, 16, "[IN 5 SECONDS]");
  Heltec.display->drawString(0, 24, "[PWROFF TO ABORT]");
  Heltec.display->display();  
  delay(5000);
  Heltec.display->clear();
  Heltec.display->drawString(0, 0, "[Running loop in 1s]");  
  Heltec.display->display();
  delay(500);
  //////////////////
  // Servo PSU 'Key'
  pinMode(ServoPSUKeyPin, OUTPUT);    
  digitalWrite(ServoPSUKeyPin,HIGH);  // Set key HIGH  
  delay(500);
  TapServoBattery();
  TapServoPSU = true;  
  LastTapServoPSU = millis();
  delay(500);  
  //////////////////
  // Init PWM Object 
  pwm.begin();  
  pwm.setOscillatorFrequency(25000000);
  pwm.setPWMFreq(SERVO_FREQ);  // 1600 is the maximum PWM frequency
  // LEFT SIDE MOVES IN REVERSE DIRECTION (or legs go wrong way)
  // ie: RIGHT side is the RIGHT way (remember)
  delay(10);
  // RESET THE LEGS
  resetFrontLeft(1);
  resetFrontRight(1);
  resetRearLeft(1);
  resetRearRight(1);
}

void loop() {    
  // try not to block anywhere else
  if ( millis() - LastTapServoPSU > StdTapInterval && TapServoPSU) {
    TapServoBattery();  // tap every 20s to keep active       
  }  
  // Move 2,3 to MAX ( 0 )
  // SWEEP TO MIN From MAX is 0 degrees 
  Heltec.display->clear();
  Heltec.display->drawString(0, 0, "[MAX TO MIN]" + String();
  Heltec.display->display();
//  for (uint16_t pulselen = SERVOMAX; pulselen > SERVOMIN+OFFSETMIN; pulselen--) {
//    pwm.setPWM(2, 0, pulselen);
//    pwm.setPWM(3, 0, pulselen);    
//  }  
//  Heltec.display->drawString(0, 24, "[MOVE DONE]");
//  Heltec.display->display();
  delay(15000);
  
  resetFrontLeft(demoFlip);
  resetFrontRight(demoFlip);
  resetRearLeft(demoFlip);
  resetRearRight(demoFlip);
  
  // Move 2,3 to MIN ( 0 )
//  // SWEEP TO MAX From MIN is 180 degrees 
//  Heltec.display->clear();
//  Heltec.display->drawString(0, 0, "[MIN TO MAX] [180]");
//  Heltec.display->drawString(0, 12, "[30s delay]");
//  Heltec.display->display();
//  for (uint16_t pulselen = SERVOMIN; pulselen < SERVOMAX-OFFSETMAX; pulselen++) {
//    pwm.setPWM(2, 0, pulselen);
//    pwm.setPWM(3, 0, pulselen);    
//  }  
//  Heltec.display->drawString(0, 24, "[MOVE DONE]");
//  Heltec.display->display();
//  delay(1000);  
}


///////////////////////
// Battery PSU Tappers
void TapServoBattery() {
    // Time for 100ms
    Heltec.display->clear();
    Heltec.display->drawString(8, 16, String("LastTapServoPSU: " + String(LastTapServoPSU/1000) + "s"));
    Heltec.display->display();
    digitalWrite(ServoPSUKeyPin,LOW);
    delay(100);
    digitalWrite(ServoPSUKeyPin,HIGH);
    LastTapServoPSU = millis();
}

void DoubleTapServoBattery() {
    // double tap turns off servo output voltage
    // Time for 250ms from here
    TapServoBattery();
    delay(50);
    TapServoBattery();
}

///////////
// Leg Work
/////////////
// Front Legs
// FRONT_L (0)
void resetFrontLeft(int rev) {
  // Reset Front_L is a SWEEP from SMIN TO SMAX (Front Left, Reverse of Right numbers (and rear function) )
  // Actual Movement is DOWN to LIMB MINIMUM
  if ( !rev ) {
    for (uint16_t pulselen = SERVOMIN; pulselen < SERVOMAX-OFFSETMAX; pulselen++) {
      pwm.setPWM(FRONT_L, 0, pulselen);    
    }  
  } else {
    for (uint16_t pulselen = SERVOMAX; pulselen < SERVOMIN+OFFSETMIN; pulselen--) {
      pwm.setPWM(FRONT_L, 0, pulselen);    
    }
  }
}

//////////////
// FRONT_R (1)
void resetFrontRight(int rev) {
  // Reset Front_R is a SWEEP from SMIN TO SMAX (Front Left, Reverse of Right numbers (and rear function) )
  // Actual Movement is DOWN to LIMB MINIMUM
  if ( !rev) {
    for (uint16_t pulselen = SERVOMAX; pulselen > SERVOMIN+OFFSETMIN; pulselen--) {
      pwm.setPWM(FRONT_R, 0, pulselen);    
    }  
  } else {
    for (uint16_t pulselen = SERVOMIN; pulselen > SERVOMAX+OFFSETMAX; pulselen++) {
      pwm.setPWM(FRONT_R, 0, pulselen);    
    }
  }
}

/////////
// REAR_L
void resetRearLeft(int rev) {
  //Set to DOWN, 90deg to body, From MAX to MIN (Clockwise)
  if ( !rev) {
    for (uint16_t pulselen = SERVOMAX; pulselen > SERVOMIN+OFFSETMIN; pulselen--) {
      pwm.setPWM(REAR_L, 0, pulselen);    
    }  
  } else {
    for (uint16_t pulselen = SERVOMIN; pulselen > SERVOMAX+OFFSETMAX; pulselen++) {
      pwm.setPWM(REAR_L, 0, pulselen);    
    }
  }  
}

/////////
// REAR_R
void resetRearRight(int rev) {
  if ( !rev ) {
    for (uint16_t pulselen = SERVOMIN; pulselen < SERVOMAX-OFFSETMAX; pulselen++) {
      pwm.setPWM(REAR_R, 0, pulselen);    
    }  
  } else {
    for (uint16_t pulselen = SERVOMAX; pulselen < SERVOMIN+OFFSETMIN; pulselen--) {
      pwm.setPWM(REAR_R, 0, pulselen);    
    }
  }  
}

////////////////////////////////////////////////////////////
/// EXAMPLES FOR SWEEPING BELOW (and other junk below that)


// bad crap
//void setServoAngle(int servoNum,int newAngle, bool direction) {
//  if ( !direction ) {
//    pwm.setPWM(servoNum,0,servoAngleToPulse(newAngle,direction));
//  } else {
//    pwm.setPWM(servoNum,0,servoAngleToPulse(newAngle,direction));
//  }
//  delay(200);
//}
//
//int servoAngleToPulse(int newAngle, bool direction) {
//  int pulse_width, analogVal;
//  if ( !direction ) {
//    pulse_width = map(newAngle,0,180,MIN_PULSE_WIDTH,MAX_PULSE_WIDTH);
//  } else {
//    pulse_width = map(newAngle,0,180,MAX_PULSE_WIDTH,MIN_PULSE_WIDTH);
//  }
//  analogVal = int (float(pulse_width) / 1000000 * SERVO_FREQ * 4096);
//  return analogVal;
//}
