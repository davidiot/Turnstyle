#include <CurieIMU.h>
#include <MadgwickAHRS.h>
#include <NewPing.h>
#include <Wire.h>
#include "rgb_lcd.h"

// parameters indicated with a * should be tuned to fit the door.

// ULTRASONIC
#define TRIGGER_PIN_1  12  // Arduino pin tied to trigger pin on the first ultrasonic sensor.
#define ECHO_PIN_1     11  // Arduino pin tied to echo pin on the first ultrasonic sensor.
#define TRIGGER_PIN_2  10  // Arduino pin tied to trigger pin on the second ultrasonic sensor.
#define ECHO_PIN_2     9  // Arduino pin tied to echo pin on the second ultrasonic sensor.

#define DETECTION_THRESH 25 // Distance cutoff for passing (< DETECTION_THRESH is considered to be a person in proximity)
#define OPEN_ANGLE 20       // Angle where door is considered open
#define MAX_DISTANCE 200    // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.
boolean doorOpen = false;   // is the door open?

// USE THIS INSTEAD TO REVERSE THE DOOR DIRECTION
// NewPing sonar_1(TRIGGER_PIN_2, ECHO_PIN_2, MAX_DISTANCE);  // NewPing setup of closer sensor.
// NewPing sonar_2(TRIGGER_PIN_1, ECHO_PIN_1, MAX_DISTANCE);  // NewPing setup of farther sensor.
NewPing sonar_1(TRIGGER_PIN_1, ECHO_PIN_1, MAX_DISTANCE);     // NewPing setup of closer sensor.
NewPing sonar_2(TRIGGER_PIN_2, ECHO_PIN_2, MAX_DISTANCE);     // NewPing setup of farther sensor.

// RGB
rgb_lcd lcd;

// MADGEWICK
// see https://www.arduino.cc/en/Tutorial/Genuino101CurieIMUOrientationVisualiser
Madgwick filter;
unsigned long microsPerReading, microsPrevious, microsBetweenReads, microsLastRead;
float accelScale, gyroScale;

// INDICATOR
const int ledPin =  13;      // the number of the on-board LED in the Arduino 101

// POPULATION
int population = 0;          // current population of the room
unsigned long closeLastTime = 0, farLastTime = 0; // The last times a person was detected at the close and far sensors, respectively.  Note that 0 means the last detection never happened or happened too long ago.

// SETUP
void setup() {
  Serial.begin(9600);

  // configure LCD
  lcd.begin(16, 2);
  lcd.setRGB(255, 255, 255);
  lcd.print("Population: 0   ");

  // start the IMU and filter
  CurieIMU.begin();
  CurieIMU.setGyroRate(25);
  CurieIMU.setAccelerometerRate(25);
  filter.begin(25);

  // Set the accelerometer range to 2G
  CurieIMU.setAccelerometerRange(2);
  // Set the gyroscope range to 250 degrees/second
  CurieIMU.setGyroRange(250);

  // initialize variables to pace updates to correct rate
  microsPerReading = 1000000 / 25;      // deals with pacing from the tutorial code
  microsPrevious = micros();            // deals with pacing from the tutorial code
  microsBetweenReads = 1000000 / 2;     // must wait half a second between consecutive reads
  microsLastRead = micros();            // time since last movement detected

  // NOTE: the initial angle of the door is always 180 degrees, so this commented code is not necessary for now.
  // get initial angle of the door
  // startAngle = measureYaw();

  // Indicate that setup is complete
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
}

void loop() {
  float doorAngle;
  int ping_1, ping_2;
  unsigned long microsNow;

  // check if it's time to read data and update the filter
  microsNow = micros();
  if (microsNow - microsPrevious >= microsPerReading) { // This condition is taken from the tutorial code -- it ensures updates happen at the right time
    float difference = measureYaw() - 180; // Normalize the measurement so that it starts from 0, since the initial angle is 180
    doorAngle = abs(difference); // Note that the abs function cannot have other operations inside of it.  This is apparently a problem with Arduino.  This line and the previous one have to be two separate operations, or this will bug up.

    if (doorAngle < OPEN_ANGLE) { // Door is closed
      closeDoor();
      Serial.println("Door not open");
    } else if (microsNow - microsLastRead < microsBetweenReads) { // Door is open, a movement was recently detected, and we are waiting between movements; do not double count or re-record the last passing time because the person may still be passing through.
      Serial.println("Movement Detected");
    } else { // Door is open, but a complete movement has not been discovered recently.
      openDoor();
      ping_1 = sonar_1.ping_cm();
      ping_2 = sonar_2.ping_cm();

      // This block and the next have similar logic.  When we detect a ping, we adjust population and reset the latest detection times if the other sensor's last detection time is recent.  Otherwise, we update the current sensor's last detection.
      if (detect(ping_2)) {
        if (closeLastTime > 0) {
          incrementPopulation();
        } else if (!detect(ping_1)) {
          farLastTime = microsNow;
        }
      } else if (detect(ping_1)) {
        if (farLastTime > 0) {
          decrementPopulation();
        } else if (!detect(ping_2)) {
          closeLastTime = microsNow;
        }
      } else { // if a significant amount of time has passed (defined as 2 * microsBetweenReads), then reset the last detection times.  This is to prevent detections from a long time ago priming the detector when it doesn't make sense.
        if (farLastTime > 0 && microsNow - farLastTime > 2 * microsBetweenReads) {
          farLastTime = 0;
        } else if (closeLastTime > 0 && microsNow - closeLastTime > 2 * microsBetweenReads) {
          closeLastTime = 0;
        }
      }
      printinfo(doorAngle, ping_1, ping_2);
    }

    // increment previous time, so we keep proper pace
    microsPrevious = microsPrevious + microsPerReading;
  }
}

float measureYaw() { // from the tutorial -- don't worry too much about this.
  int aix, aiy, aiz;
  int gix, giy, giz;
  float ax, ay, az;
  float gx, gy, gz;
  // read raw data from CurieIMU
  CurieIMU.readMotionSensor(aix, aiy, aiz, gix, giy, giz);

  // convert from raw data to gravity and degrees/second units
  ax = convertRawAcceleration(aix);
  ay = convertRawAcceleration(aiy);
  az = convertRawAcceleration(aiz);
  gx = convertRawGyro(gix);
  gy = convertRawGyro(giy);
  gz = convertRawGyro(giz);

  // update the filter, which computes orientation
  filter.updateIMU(gx, gy, gz, ax, ay, az);

  return filter.getYaw();
}

void incrementPopulation() {
  population++;
  updatePopulationAndMoveCursor();
  lcd.write("Welcome!        ");
}

void decrementPopulation() {
  if (population > 0) { // whoops
    population--;
  }
  updatePopulationAndMoveCursor();
  lcd.write("Have a nice day!");
}

void openDoor() { // only open the door if it wasn't already opened
  if (!doorOpen) {
    lcd.setRGB(0, 255, 0);
    doorOpen = true;
  }
}

void closeDoor() { // only close the door if it wasn't already closed.
  if (doorOpen) {
    lcd.setRGB(255, 255, 255);
    doorOpen = false;
  }
}

// Update population and move cursor to where the message will be written
void updatePopulationAndMoveCursor() {
  lcd.setCursor(12, 0);
  lcd.write("    ");
  lcd.setCursor(12, 0);
  lcd.print(population);
  lcd.setCursor(0, 1);
  farLastTime = 0;
  closeLastTime = 0;
  microsLastRead = micros();
}

// from tutorial
float convertRawAcceleration(int aRaw) {
  // since we are using 2G range
  // -2g maps to a raw value of -32768
  // +2g maps to a raw value of 32767
  
  float a = (aRaw * 2.0) / 32768.0;
  return a;
}

// from tutorial
float convertRawGyro(int gRaw) {
  // since we are using 250 degrees/seconds range
  // -250 maps to a raw value of -32768
  // +250 maps to a raw value of 32767
  
  float g = (gRaw * 250.0) / 32768.0;
  return g;
}

// did we get a detection?  boolean logic goes here to avoid doing >0 checks (which indicate that the ping was too far away)
boolean detect(int pingValue) {
  return pingValue < DETECTION_THRESH && pingValue > 0;
}

void printinfo(float doorAngle, int ping_1, int ping_2) {
  Serial.print("Door Angle: ");
  Serial.print(doorAngle);
  Serial.print(" | Ping 1: ");
  Serial.print(ping_1); // Send ping, get distance in cm and print result (0 = outside set distance range)
  Serial.print(" cm | Ping 2: ");
  Serial.print(ping_2); // Send ping, get distance in cm and print result (0 = outside set distance range)
  Serial.println(" cm");
}
