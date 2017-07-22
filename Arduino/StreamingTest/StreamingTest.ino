#include "rgb_lcd.h"

rgb_lcd lcd;
int buttonPin = 4; 
int buzzerPin = 8;

int beeps = 0;
boolean repLock = false; // prevents double counting

void setup() {
  Serial.begin(9600);

  // configure LCD
  lcd.begin(16, 2);
  lcd.setRGB(255, 255, 255);
  lcd.write("BEEPS: ");

  // configure pins
  pinMode(buttonPin,INPUT);   //set button as digital input
  pinMode(buzzerPin,OUTPUT);  //as buzzer as digital output
}

void loop() {
  if (digitalRead(buttonPin)) /*check button is pressed or not */ {
    digitalWrite(buzzerPin, HIGH);  //pressed，then buzzer buzzes
    if (!repLock) {
      beeps++;
      Serial.print(beeps);
      Serial.println("*"); // we use * as a delimiter because there is some weird glitch with Node creating extra newlines
      lcd.setCursor(7, 0);
      lcd.print(beeps);
      repLock = true;
    }
  } else {
    digitalWrite(buzzerPin, LOW);//not pressed，then buzzer remains silent
    repLock = false;
  }
}
