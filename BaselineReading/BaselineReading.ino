#include <CurieIMU.h>
#include <MadgwickAHRS.h>
#include <NewPing.h>

// ULTRASONIC
#define TRIGGER_PIN_1  12  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN_1     11  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define TRIGGER_PIN_2  10  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN_2     9  // Arduino pin tied to echo pin on the ultrasonic sensor.

#define DETECTION_THRESH 50 // Distance cutoff for passing
#define MAX_DISTANCE 200 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

NewPing sonar_1(TRIGGER_PIN_1, ECHO_PIN_1, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
NewPing sonar_2(TRIGGER_PIN_2, ECHO_PIN_2, MAX_DISTANCE); // NewPing setup of pins and maximum distance.

// MADGEWICK
Madgwick filter;
unsigned long microsPerReading, microsPrevious;
float accelScale, gyroScale;

// BUTTON
// set pin numbers:
const int buttonPin = 2;     // the number of the pushbutton pin
const int ledPin =  13;      // the number of the LED pin

int buttonState = 0;         // variable for reading the pushbutton status

// BASELINE
const int readyPin = 3;

// SETUP
void setup() {
  Serial.begin(9600);

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
  microsPerReading = 1000000 / 25;
  microsPrevious = micros();

  // initialize the LED pin as an output:
  pinMode(ledPin, OUTPUT);
  // initialize the pushbutton pin as an input:
  pinMode(buttonPin, INPUT);
}

void loop() {
  // read the state of the pushbutton value:
  buttonState = digitalRead(buttonPin);

  // check if the pushbutton is pressed.
  // if it is, the buttonState is HIGH:
  if (buttonState == HIGH) {
    // turn LED on:
    digitalWrite(ledPin, HIGH);
    digitalWrite(readyPin, HIGH);
  } else {
    // turn LED off:
    digitalWrite(ledPin, LOW);
    int aix, aiy, aiz;
    int gix, giy, giz;
    float ax, ay, az;
    float gx, gy, gz;
    float doorAngle;
    int ping_1, ping_2;
    unsigned long microsNow;
  
    // check if it's time to read data and update the filter
    microsNow = micros();
    if (microsNow - microsPrevious >= microsPerReading) {
  
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
  
      // print the heading, pitch and roll
      doorAngle = filter.getYaw();
      Serial.print("Door Angle: ");
      Serial.println(doorAngle);
  
      ping_1 = sonar_1.ping_cm();
      ping_2 = sonar_2.ping_cm();
      printpings(ping_1, ping_2);
  
      // increment previous time, so we keep proper pace
      microsPrevious = microsPrevious + microsPerReading;
    }
  }
}

float convertRawAcceleration(int aRaw) {
  // since we are using 2G range
  // -2g maps to a raw value of -32768
  // +2g maps to a raw value of 32767
  
  float a = (aRaw * 2.0) / 32768.0;
  return a;
}

float convertRawGyro(int gRaw) {
  // since we are using 250 degrees/seconds range
  // -250 maps to a raw value of -32768
  // +250 maps to a raw value of 32767
  
  float g = (gRaw * 250.0) / 32768.0;
  return g;
}

void printpings(int ping_1, int ping_2) {
  Serial.print("Ping 1: ");
  Serial.print(ping_1); // Send ping, get distance in cm and print result (0 = outside set distance range)
  Serial.print(" cm | Ping 2: ");
  Serial.print(ping_2); // Send ping, get distance in cm and print result (0 = outside set distance range)
  Serial.println(" cm");
}
