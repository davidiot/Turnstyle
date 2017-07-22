// ---------------------------------------------------------------------------
// Example NewPing library sketch that does a ping about 20 times per second.
// ---------------------------------------------------------------------------

#include <NewPing.h>

#define TRIGGER_PIN_1  12  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN_1     11  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define TRIGGER_PIN_2  10  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN_2     9  // Arduino pin tied to echo pin on the ultrasonic sensor.

#define DETECTION_THRESH 50 // Distance cutoff for passing
#define MAX_DISTANCE 200 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

NewPing sonar_1(TRIGGER_PIN_1, ECHO_PIN_1, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
NewPing sonar_2(TRIGGER_PIN_2, ECHO_PIN_2, MAX_DISTANCE); // NewPing setup of pins and maximum distance.

int counter = 0;

void setup() {
  Serial.begin(115200); // Open serial monitor at 115200 baud to see ping results.
}

void loop() {
  delay(50);                     // Wait 50ms between pings (about 20 pings/sec). 29ms should be the shortest delay between pings.
  int ping_1 = sonar_1.ping_cm();
  int ping_2 = sonar_2.ping_cm();
  printpings(ping_1, ping_2);

  counter = (counter + 1) % 5;
}

void printpings(int ping_1, int ping_2) {
  Serial.print("Ping 1: ");
  Serial.print(ping_1); // Send ping, get distance in cm and print result (0 = outside set distance range)
  Serial.print(" cm | Ping 2: ");
  Serial.print(ping_2); // Send ping, get distance in cm and print result (0 = outside set distance range)
  Serial.println(" cm");
}
