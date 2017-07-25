#include "rgb_lcd.h"
#include <CurieBLE.h>

rgb_lcd lcd;

BLEPeripheral blePeripheral;
BLEService turnstyleBleService("180D"); // just need a service that transmits a number

BLECharacteristic turnstyleBleIntCharacteristic("2A37", BLERead | BLENotify, 2);

int buttonPin = 4;
int buzzerPin = 8;

int beeps = 0;
boolean repLock = false; // prevents double counting

void setup() {
  Serial.begin(9600);
  blePeripheral.setLocalName("Turnstyle");
  blePeripheral.setAdvertisedServiceUuid(turnstyleBleService.uuid());
  blePeripheral.addAttribute(turnstyleBleService);
  blePeripheral.addAttribute(turnstyleBleIntCharacteristic);
  blePeripheral.begin();
  Serial.println("Bluetooth device active, waiting for connections...");

  // configure LCD
  lcd.begin(16, 2);
  lcd.setRGB(255, 255, 255);
  lcd.write("BEEPS: ");

  // configure pins
  pinMode(buttonPin, INPUT);  //set button as digital input
  pinMode(buzzerPin, OUTPUT); //as buzzer as digital output
}

void loop() {
  // listen for BLE peripherals to connect:
  BLECentral central = blePeripheral.central();
  // if a central is connected to peripheral:
  if (central) {
    Serial.print("Connected to central: ");
    // print the central's MAC address:
    Serial.println(central.address());
    // turn on the LED to indicate the connection:
    digitalWrite(13, HIGH);
    // as long as the central is still connected:
    while (central.connected()) {
      loopHelper(true);
    }
    // when the central disconnects, turn off the LED:
    digitalWrite(13, LOW);
    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
  } else {
    loopHelper(false);
  }
}

void loopHelper(boolean connected) {
  if (digitalRead(buttonPin)) { /*check button is pressed or not */
    digitalWrite(buzzerPin, HIGH);  //pressed，then buzzer buzzes
    if (!repLock) {
      beeps++;
      Serial.print(beeps);
      Serial.println("*"); // we use * as a delimiter because there is some weird glitch with Node creating extra newlines
      lcd.setCursor(7, 0);
      lcd.print(beeps);
      repLock = true;
      boolean thresh = (beeps > 10);
      const unsigned char dataArray[2] = { (char) beeps, (char) ( ((beeps >> 8) & 0x7F) | (thresh ? 0x80 : 0x00) )};
      turnstyleBleIntCharacteristic.setValue(dataArray, 2);
    }
  } else {
    digitalWrite(buzzerPin, LOW);//not pressed，then buzzer remains silent
    repLock = false;
  }
}
