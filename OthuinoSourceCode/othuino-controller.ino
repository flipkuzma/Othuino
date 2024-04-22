#include "Arduino.h"
#include "LiquidCrystal.h"

const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
int player = 0;
String pos = "";
static int joystickXOld;
static int joystickYOld;
static bool buttonValOld;

int joystickXNew;
int joystickYNew;
int buttonValNew;

String s1;
String s2;

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  pinMode(A3, INPUT); //joystick x axis
  pinMode(A1, INPUT); //joystick y axis
  pinMode(6, INPUT_PULLUP); //pushbutton 
  analogWrite(10, 20); // contrast value

  lcd.print("Press to start");
  while (digitalRead(6) == HIGH) {
    if (Serial.available() > 0) {
      Serial.write(Serial.read());
    }
  }
  lcd.clear();
  Serial.write(202);
  lcd.print("Waiting...");
  int ready = 0;
  while (!ready) {
    while (Serial.available() == 0) {}
    int byte = Serial.read();
    if (byte == 200) {
      ready = 1;
    }

    Serial.write(byte);
  }
  
}

void printStatus(){
  lcd.clear();
  lcd.setCursor(0, 0); // set cursor position to second line
  lcd.print(s1);
  lcd.setCursor(0, 1);
  lcd.print(s2);
  // if (player == 0) {
  //   lcd.print("Player 1's turn");
  // } else {
  //   lcd.print("Player 2's turn");
  // }
}

void loop() {
  if (Serial.available() > 0) {
    int byte = Serial.read(); //should be 7
    if (byte == 7) {
      int player = Serial.read();
      int score1 = Serial.read();
      int score2 = Serial.read();

      if (player == 15) {
        s1 = "Tie...";
        s2 = "Reset game!";
      } else if (player == 16) {
        s1 = "Player 1 wins!";
        s2 = "Reset game!";
      } else if (player == 17) {
        s1 = "Player 2 wins!";
        s2 = "Reset game!";
      } else {
        s1 = "Player " + String(player) + ", GO!";    
        s2 = "Score: " + String(score1) + " - " + String(score2);
      }

      printStatus();
    } else if (byte == 8) {
      lcd.clear();
      lcd.print("Occupied!");
      delay(1500);
      printStatus();
    } else if (byte == 9) {
      lcd.clear();
      lcd.print("Illegal!");
      printStatus();
    } else {
      Serial.write(byte); //Sending Filip byte other than 7
    }
  }

  joystickXOld = 0;
  joystickYOld = 0;
  buttonValOld = HIGH;

  joystickXNew = analogRead(A3);
  joystickYNew = analogRead(A1);
  buttonValNew = digitalRead(6);

  if (analogRead(A3) < 300) {
    // Serial.println("Up");
    // pos = "Up";
    Serial.write(5); // made a move
    Serial.write(0); // UP
    delay(125);
  } else if (analogRead(A3) > 700) {
    // Serial.println("Down");
    // pos = "Down";
    Serial.write(5); // made a move
    Serial.write(2); // DOWN
    delay(125);
  } else if (analogRead(A1) > 700 ) {
    // Serial.println("Left");
    // pos = "Left";
    Serial.write(5); // made a move
    Serial.write(1); // RIGHT
    delay(125);
  } else if (analogRead(A1) < 300) {
    // Serial.println("Right");
    // pos = "Right";
    Serial.write(5); // made a move
    Serial.write(3); // LEFT
    delay(125);
  }

  if (buttonValNew == LOW && buttonValOld == HIGH) {
    delay(50);
    Serial.write(6); //button is pressed
    Serial.write(0); //garbage value
    //Serial.println("Button Pressed");
    if (player == 0) {
      player = 1;
    } else {
      player = 0;
    }
  }

  joystickXOld = joystickXNew;
  joystickYOld = joystickYNew;
  buttonValOld = buttonValNew;

  delay(100);
}