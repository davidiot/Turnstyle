#include <CurieIMU.h>
#include <MadgwickAHRS.h>
#include <NewPing.h>
#include <Wire.h>
#include <Adafruit_MCP23017.h>
#include <Adafruit_RGBLCDShield.h>
#include <CurieBLE.h>

#define TRIGGER_PIN_1  12  // Arduino pin tied to trigger pin on the first ultrasonic sensor.
#define ECHO_PIN_1     11  // Arduino pin tied to echo pin on the first ultrasonic sensor.
#define TRIGGER_PIN_2  10  // Arduino pin tied to trigger pin on the second ultrasonic sensor.
#define ECHO_PIN_2     9   // Arduino pin tied to echo pin on the second ultrasonic sensor.
#define SWITCH_PIN     A2  // Arduino pin tied to magnetic contact switch lead

#define DETECTION_THRESH 25 // Distance cutoff for passing (< DETECTION_THRESH is considered to be a person in proximity)***
#define OPEN_ANGLE 20       // Angle where door is considered open***
#define MAX_DISTANCE 200    // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

// ULTRASONIC -- settings match for blackboard-hardware V1.0
NewPing sonar_1(TRIGGER_PIN_1, ECHO_PIN_1, MAX_DISTANCE);
NewPing sonar_2(TRIGGER_PIN_2, ECHO_PIN_2, MAX_DISTANCE);
NewPing closeSonar = sonar_1;                               // NewPing setup of closer sensor.
NewPing farSonar = sonar_2;                                 // NewPing setup of farther sensor.
boolean enterFromLeftToRight = true;                        // is leftToRight movement considered entering or vice-versa?

// RGB_LCD from Adafruit
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

// BLUETOOTH
BLEPeripheral blePeripheral;
BLEService turnstyleBleService("180D"); // just need a service that transmits a number
BLECharacteristic turnstyleBleIntCharacteristic("2A37", BLERead | BLENotify, 2);

// MADGEWICK
// see https://www.arduino.cc/en/Tutorial/Genuino101CurieIMUOrientationVisualiser
Madgwick filter;
unsigned long microsPerReading, microsPrevious, microsBetweenReads, microsLastRead;
float accelScale, gyroScale;

// MAGNETIC SWITCH BOOLEAN AND BASELINE CALIBRATION FOR IMU
boolean isMagSwitchOpen = false;                     // true/false about the state of the door according to the magnetic contact switch
boolean isDoorOpen = false;                          // is the door open?  We define an openDoor to be BOTH isMagSwitchOpen AND doorAngle > OPEN_ANGLE, as opposed to just isMagSwitchOpen
boolean switchClosedLastIteration = true;            // hysteresis/state change memory from closed to open for calibration
float baselineYaw;                                   // global variable declaration of the baseline Yaw was the only way to get it to exist outside of consecutive loops-- otherwise it gets erased

// INDICATOR
const int ledPin =  13;      // the number of the on-board LED in the Arduino 101

// POPULATION
int population = 0;          // current population of the room
unsigned long closeLastTime = 0, farLastTime = 0; // The last times a person was detected at the close and far sensors, respectively.  Note that 0 means the last detection never happened or happened too long ago.

// FOR DEBUGGING -- SET TO FALSE TO USE WITH NODE SERVER
boolean debug = true;

// SETUP
void setup() {
  Serial.begin(9600);

  // configure Bluetooth
  blePeripheral.setLocalName("Turnstyle");
  blePeripheral.setAdvertisedServiceUuid(turnstyleBleService.uuid());
  blePeripheral.addAttribute(turnstyleBleService);
  blePeripheral.addAttribute(turnstyleBleIntCharacteristic);
  blePeripheral.begin();
  printlnIfDebug("Bluetooth device active, waiting for connections...");

  // configure LCD
  lcd.begin(16, 2);
  lcd.setBacklight(0x7);    // sets backlight to brightest setting (white)
  lcd.print("Population: 0   ");

  // establish pinMode for magnetic contact switch
  pinMode(SWITCH_PIN, INPUT); // if analogRead pulls high, then switch is closed. else, door is open.

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

  // Indicate that setup is complete
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
}

void loop() {
  // listen for BLE peripherals to connect:
  BLECentral central = blePeripheral.central();
  // if a central is connected to peripheral:
  if (central) {
    printIfDebug("Connected to central: ");
    // print the central's MAC address:
    printlnIfDebug(central.address());
    // turn on the LED to indicate the connection:
    digitalWrite(13, HIGH);
    // as long as the central is still connected:
    while (central.connected()) {
      loopHelper(true);
    }
    // when the central disconnects, turn off the LED:
    digitalWrite(13, LOW);
    printIfDebug("Disconnected from central: ");
    printlnIfDebug(central.address());
  } else {
    loopHelper(false);
  }
}

void loopHelper(boolean connected) {  // The code in this function is basically what would appear in loop() without bluetooth
  float doorAngle;
  int ping_1, ping_2;
  unsigned long microsNow;

  // check if it's time to read data and update the filter
  microsNow = micros();
  if (microsNow - microsPrevious >= microsPerReading) { // This condition is taken from the tutorial code -- it ensures updates happen at the right time

    // Checks the magnetic switch
    isMagSwitchOpen = (analogRead(SWITCH_PIN) < 800);

    if (isMagSwitchOpen) {
      float currentYaw = measureYaw();
      if (switchClosedLastIteration) {
        baselineYaw = currentYaw;
        switchClosedLastIteration = false;
      }
      float doorAngle = abs(currentYaw - baselineYaw);
      // if the doorAngle is > 180, it must be due to the discontinuity, assuming doors open to an angle of max 180 degrees.
      if (doorAngle > 180) {
        doorAngle = 360 - doorAngle;
      }
    } else {
      switchClosedLastIteration = true;
    }

    if (!isMagSwitchOpen || doorAngle < OPEN_ANGLE) { // Door is closed
      closeDoor();
    } else if ((microsNow - microsLastRead < microsBetweenReads)) { // Door is open, a movement was recently detected, and we are waiting between movements; do not double count or re-record the last passing time because the person may still be passing through.
      printlnIfDebug("Movement Detected");
    } else { // Door is open, but a complete movement has not been discovered recently.
      openDoor();
      ping_1 = closeSonar.ping_cm();
      ping_2 = farSonar.ping_cm();

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
      printInfoIfDebug(doorAngle, ping_1, ping_2);
    }

    // Only bother updating bluetooth data if a device is connected
    if (connected) {
      updateBleCharacteristic();
    }

    // CURRENT CONTROL SCHEME: left and right buttons make the entering direction in the orientation specified by the button.
    // select buttons always toggles the orientation.
    uint8_t buttons = lcd.readButtons();
    if (buttons) {
      if (buttons & BUTTON_LEFT) {
        if (enterFromLeftToRight) {
          swapSonars();
        }
      }
      if (buttons & BUTTON_RIGHT) {
        if (!enterFromLeftToRight) {
          swapSonars();
        }
      }
      if (buttons & BUTTON_SELECT) {
        swapSonars();
      }
    }

    // increment previous time, so we keep proper pace
    microsPrevious = microsPrevious + microsPerReading;
  }
}

// Flip the orientation of the sensors
void swapSonars() {
  NewPing temp = closeSonar;
  closeSonar = farSonar;
  farSonar = temp;
  enterFromLeftToRight = !enterFromLeftToRight;
  String message = enterFromLeftToRight ? "Enter this way->" : "<-Enter this way";
  displayMessage(message);
}

void updateBleCharacteristic() {
  const unsigned char dataArray[2] = { (char) population, (char) ( ((population >> 8) & 0x7F) | (isDoorOpen ? 0x80 : 0x00) )};
  turnstyleBleIntCharacteristic.setValue(dataArray, 2);
}

void incrementPopulation() {
  population++;
  updatePopulation();
  displayMessage("Welcome!        ");
}

void decrementPopulation() {
  if (population > 0) { // whoops
    population--;
  }
  updatePopulation();
  displayMessage("Have a nice day!");
}

void openDoor() { // only open the door if it wasn't already opened
  if (!isDoorOpen) {
    isDoorOpen = true;
    printlnIfDebug("Door open");
  }
}

void closeDoor() { // only close the door if it wasn't already closed.
  if (isDoorOpen) {
    isDoorOpen = false;
    printlnIfDebug("Door closed");
  }
}

// Update population
void updatePopulation() {
  lcd.setCursor(12, 0);
  lcd.print("    ");
  lcd.setCursor(12, 0);
  lcd.print(population);

  printIfDebug("Population: ");
  Serial.print(population);
  if (!debug) {
    Serial.println("*");
  } else {
    Serial.println();
  }
  farLastTime = 0;
  closeLastTime = 0;
  microsLastRead = micros();
}

// displays a message in the second line of the lcd.  Use for greetings/farewells and calibrations
void displayMessage(String message) {
  lcd.setCursor(0, 1);
  lcd.print(message);
}

// did we get a detection?  boolean logic goes here to avoid doing >0 checks (which indicate that the ping was too far away)
boolean detect(int pingValue) {
  return pingValue < DETECTION_THRESH && pingValue > 0;
}

// prints measurement info
void printInfoIfDebug(float doorAngle, int ping_1, int ping_2) {
  if (debug) {
    Serial.print("Door Angle: ");
    Serial.print(doorAngle);
    Serial.print(" | Ping 1: ");
    Serial.print(ping_1); // Send ping, get distance in cm and print result (0 = outside set distance range)
    Serial.print(" cm | Ping 2: ");
    Serial.print(ping_2); // Send ping, get distance in cm and print result (0 = outside set distance range)
    Serial.println(" cm");
  }
}

// wrap all Serial.prints using this function for ease of debugging
void printIfDebug(String message) {
  if (debug) {
    Serial.print(message);
  }
}

// wrap all Serial.printlns using this function for ease of debugging
void printlnIfDebug(String message) {
  if (debug) {
    Serial.println(message);
  }
}

// from the tutorial -- don't worry too much about this.
float measureYaw() {
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
