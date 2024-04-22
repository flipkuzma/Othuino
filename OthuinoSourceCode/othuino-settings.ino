// Settings Modifier

#include <LiquidCrystal.h>
#include <string.h>

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
int page = 1;
int row = 0;

String settings[] = { "CHANGE COLOR", /*"TIMER",*/"BOARD POS.", "RESET GAME" };
String colors[] = { "RED/GREEN", "RED/BLUE", "BLUE/YELLOW", "BACK" };
String times[] = { "NO LIMIT", "5 SEC/TURN", "30 SEC/TURN", "BACK" };
String boardPos[] = { "LEFT", "CENTER", "RIGHT", "BACK" };

// purposes
// 1 - color
// 2 - time
// 3 - position
// 4 - reset

// send info to LCD display
void renderScroller(String strings[], int x) {
  lcd.setCursor(0, 0);
  lcd.print(strings[row]);
  lcd.print(" <-");

  if (row < x) {
    lcd.setCursor(0, 1);
    lcd.print(strings[row + 1]);
  }
}

// send info to LCD display
void render() {
  lcd.clear();
  if (page == 1) {
    renderScroller(settings, 2);
  } else if (page == 2) {
    renderScroller(colors, 3);
  } /*else if (page == 3) {
    renderScroller(times);
  } */else if (page == 4) {
    renderScroller(boardPos, 3);
  }
}

// initialize
void setup() {
  Serial.begin(9600);
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  analogWrite(10, 80);

  lcd.print("Press to start");

  while (digitalRead(6) == HIGH) {
    if (Serial.available() > 0) {
      Serial.write(Serial.read());
    }
  }
  
  lcd.clear();
  
  Serial.write(203);

  lcd.print("Waiting...");
  
  int ready = 0;
  while (!ready) {
    while (Serial.available() == 0) {}
    int byte = Serial.read();
    
    if (byte == 200) {
      ready = 1;
    } else if (byte == 202) {
      Serial.write(202);
    }
  }

  render();

  delay(500);
}

// register a button click
void click() {
  if (page == 1) {
    if (row == 0) {
      page = 2;
      render();
    } else if (row == 1) {
      page = 4;
      row = 0;
      render();
    } else if (row == 2) {
      Serial.write(4);
      Serial.write(0);

      page = 1;
      row = 0;
      render();
    }
  } else if (page == 2) {  // change color
    if (row == 3) {
      page = 1;
      row = 0;
      render();
    } else {
      Serial.write(1);
      Serial.write(row);

      page = 1;
      row = 0;
      render();
    }
  } else if (page == 4) {
    if (row == 3) {
      page = 1;
      row = 0;
      render();
    } else {
      Serial.write(3);
      Serial.write(row);

      page = 1;
      row = 0;
      render();
    }
  }

  delay(650);
}

void loop() {
  if (digitalRead(6) == LOW) {
    click();

    return;
  }

  if (analogRead(A1) > 800) {
    if (row > 0) {
      row--;
      render();
      delay(250);
    }
  } else if (analogRead(A1) < 200) {
    if (page == 1 && row == 2) return;

    if (row < 3) {
      row++;
      render();
      delay(250);
    }
  }

  while (Serial.available() > 0) {
    Serial.write(Serial.read());
    //Serial.println(byte);
  }
}