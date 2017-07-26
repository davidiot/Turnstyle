#include "rgb_lcd.h"
#include <CurieBLE.h>

rgb_lcd lcd;

BLEService turnstyleService("468db76d-4b92-48a4-8727-426f9a4a2482");
BLEUnsignedIntCharacteristic BlePopulationCharacteristic("d75b671b-6ea4-464e-89fd-1ab8ad76440b", BLERead | BLEWrite | BLENotify);
BLEUnsignedCharCharacteristic BleOpenCharacteristic("8404e92d-0ca7-480b-8b3f-7a1e4c8406f1", BLERead | BLENotify); // See https://github.com/01org/corelibs-arduino101/issues/554 for why we don't use BleBoolCharacteristic

int buttonPin = 4;
int buzzerPin = 8;

unsigned int beeps = 0;
boolean repLock = false; // prevents double counting

void setup() {
  Serial.begin(9600);
  BLE.begin();
  BLE.setLocalName("TSTYLE");
  BLE.setAdvertisedService(turnstyleService);
  turnstyleService.addCharacteristic(BlePopulationCharacteristic);
  turnstyleService.addCharacteristic(BleOpenCharacteristic);
  BLE.addService(turnstyleService);
  BlePopulationCharacteristic.setValue(0);
  BleOpenCharacteristic.setValue(0);
  BLE.advertise();
  
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
  BLEDevice central = BLE.central();
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
      //const unsigned char dataArray[2] = { (char) beeps, (char) ( ((beeps >> 8) & 0x7F) | (thresh ? 0x80 : 0x00) )};
      //turnstyleBleIntCharacteristic.setValue(dataArray, 2);
      BlePopulationCharacteristic.setValue(beeps);
      BleOpenCharacteristic.setValue((int) thresh);
    }
  } else {
    digitalWrite(buzzerPin, LOW);//not pressed，then buzzer remains silent
    repLock = false;
  }
}
