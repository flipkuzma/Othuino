// testshapes demo for Adafruit RGBmatrixPanel library.
// Demonstrates the drawing abilities of the RGBmatrixPanel library.
// For 16x32 RGB LED matrix:
// http://www.adafruit.com/products/420

// Written by Limor Fried/Ladyada & Phil Burgess/PaintYourDragon
// for Adafruit Industries.
// BSD license, all text above must be included in any redistribution.

#include <RGBmatrixPanel.h>

// Most of the signal pins are configurable, but the CLK pin has some
// special constraints.  On 8-bit AVR boards it must be on PORTB...
// Pin 8 works on the Arduino Uno & compatibles (e.g. Adafruit Metro),
// Pin 11 works on the Arduino Mega.  On 32-bit SAMD boards it must be
// on the same PORT as the RGB data pins (D2-D7)...
// Pin 8 works on the Adafruit Metro M0 or Arduino Zero,
// Pin A4 works on the Adafruit Metro M4 (if using the Adafruit RGB
// Matrix Shield, cut trace between CLK pads and run a wire to A4).

#define CLK  8   // USE THIS ON ARDUINO UNO, ADAFRUIT METRO M0, etc.
//#define CLK A4 // USE THIS ON METRO M4 (not M0)
//#define CLK 11 // USE THIS ON ARDUINO MEGA
#define OE   9
#define LAT 10
#define A   A0
#define B   A1
#define C   A2

RGBmatrixPanel matrix(A, B, C, CLK, LAT, OE, false);

int posX;
int posY;
int player;

int score1;
int score2;

int colorCode;
int orient;

int timeLimit;

uint16_t colors[3][3] = {{matrix.Color333(0, 0, 0), matrix.Color333(2, 0, 0), matrix.Color333(0, 2, 0)},
                          {matrix.Color333(0, 0, 0), matrix.Color333(2, 0, 0), matrix.Color333(0, 0, 2)},
                          {matrix.Color333(0, 0, 0), matrix.Color333(2, 2, 0), matrix.Color333(0, 0, 2)}};

uint16_t cursorColor = matrix.Color333(2, 2, 2);
uint16_t edgeColor = matrix.Color333(0, 2, 2);
uint16_t brightEdge = matrix.Color333(7, 7, 7);

int board[8][8];

// PROTOCOL CODES
//   single byte
//     200 = both confirmations received, board subsystem ready
//     201 = board resetting, please send confirmations again (give 3 seconds maybe?)
//     202 = controller subsystem ready
//     203 = settings subsystem ready
//   two bytes - second byte is variable data
//     1 = change color
//     2 = change time limit
//     3 = change board offset
//     4 = reset game
//     5 = joystick move
//     6 = commit move
//     7 = set controller message
//     8 = occupied space
//     9 = illegal move

// Get the correct color based on the current selection
uint16_t getColor(int player) {
  return colors[colorCode][player];
} 

// Render a single square with a certain color
void renderSquare(int x, int y, uint16_t color) {
  matrix.fillRect(orient + 2 * x, 2 * y, 2, 2, color);
  //delay(100);
}

// Render a single square with the appropriate color
void renderSquare(int x, int y) {
  //Serial.println(getColor(board[x][y]));
  renderSquare(x, y, getColor(board[x][y]));
}

// Render the edge barriers that make the display prettier
void renderEdges() {
  if (orient == 0) {
    matrix.fillRect(17, 0, 15, 16, getColor(player));
  } else if (orient == 8) {
    matrix.fillRect(0, 0, 7, 16, getColor(player == 1 ? player : edgeColor));
    matrix.fillRect(25, 0, 7, 16, getColor(player == 2 ? player : edgeColor));
  } else if (orient == 16) {
    matrix.fillRect(0, 0, 15, 16, getColor(player));
  }
}

// Re-render the entire screen
// Only do this if settings are changed- inefficient for small changes
void render() {
  matrix.fillScreen(matrix.Color333(0, 0, 0));

  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      renderSquare(i, j);
    }
  }

  renderSquare(posX, posY, cursorColor);

  renderEdges();
}

// Setup the beginning of the game
void setup() {
  matrix.begin();

  // Wipe screen
  matrix.fillScreen(matrix.Color333(0, 0, 0));

  Serial.begin(9600);

  // Display two red pixels, showing current status of subsystems
  matrix.drawPixel(0, 0, matrix.Color333(3, 0, 0));
  matrix.drawPixel(1, 0, matrix.Color333(3, 0, 0));

  // block until both subsystems have started
  int ready2 = 0;
  int ready3 = 0;

  // loop until both are ready
  while (!ready2 || !ready3) {
    while (Serial.available() == 0) {} // wait for comm

    int a = Serial.read();

    if (a == 202) { // subsystem 2 is ready
      ready2 = 1;
    } else if (a == 203) { // subsystem 3 is ready
      ready3 = 1;
    }

    // if either are ready, render pixel green for user info
    if (ready2) matrix.drawPixel(0, 0, matrix.Color333(0, 3, 0));
    if (ready3) matrix.drawPixel(1, 0, matrix.Color333(0, 3, 0));
  }

  // clear the status pixels
  matrix.drawPixel(0, 0, matrix.Color333(0, 0, 0));
  matrix.drawPixel(1, 0, matrix.Color333(0, 0, 0));

  // both subsystems are on, sending a confirmation back

  Serial.write(200);

  posX = 0;
  posY = 0;
  player = 1;
  score1 = 0;
  score2 = 0;
  colorCode = 0;
  orient = 8;
  timeLimit = 0;

  sendScore();

  // empty the board array
  for (int i = 0; i < 7; i++) {
    for (int j = 0; j < 7; j++) {
      board[i][j] = 0;
    }
  }

  // place starting chips
  board[3][3] = 1;
  board[3][4] = 2;
  board[4][3] = 2;
  board[4][4] = 1;

  // do a full render
  render();
}

// 0 - good move
// 1 - space occupied
int checkMove() {
  if (board[posX][posY] != 0) {
    return 1;
  }

  return 0;
}

// reset the game
void reset() {
  posX = 0;
  posY = 0;
  player = 1;
  score1 = 0;
  score2 = 0;

  // empty board array
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      board[i][j] = 0;
    }
  }

  // place starting chips
  board[3][3] = 1;
  board[3][4] = 2;
  board[4][3] = 2;
  board[4][4] = 1;

  render();
}

void sendScore() {
  Serial.write(7);
  Serial.write(player);
  Serial.write(score1);
  Serial.write(score2);
}

// 0 - good move
// 1 - not legal
// calculate logic!
int commit() {
  int legal = 0;

  int f1 = 0;

  int oppPlayer = player == 1 ? 2 : 1;

  for (int i = posX - 1; i >= 0; i--) {
    if (f1 == 0) {
      if (board[i][posY] == oppPlayer) {
        f1 = 1;
        continue;
      }

      break;
    }

    if (board[i][posY] == 0) {
      break;
    } else if (board[i][posY] == player) {
      legal = 1;

      for (int x = posX; x > i; x--) {
        board[x][posY] = player;
        matrix.fillRect(orient + 2 * x, 2 * posY, 2, 2, getColor(player));
        delay(175);
      }

      break;
    }
  }
  
  f1 = 0;

  for (int i = posX + 1; i <= 15; i++) {
    if (f1 == 0) {
      if (board[i][posY] == oppPlayer) {
        f1 = 1;
        continue;
      }

      break;
    }

    if (board[i][posY] == 0) {
      break;
    } else if (board[i][posY] == player) {
      legal = 1;

      for (int x = posX; x < i; x++) {
        board[x][posY] = player;
        matrix.fillRect(orient + 2 * x, 2 * posY, 2, 2, getColor(player));
        delay(175);
      }

      break;
    }
  }

  f1 = 0;

  for (int i = posY + 1; i <= 15; i++) {
    if (f1 == 0) {
      if (board[posX][i] == oppPlayer) {
        f1 = 1;
        continue;
      }

      break;
    }

    if (board[posX][i] == 0) {
      break;
    } else if (board[posX][i] == player) {
      legal = 1;

      for (int y = posY; y < i; y++) {
        board[posX][y] = player;
        matrix.fillRect(orient + 2 * posX, 2 * y, 2, 2, getColor(player));
        delay(175);
      }

      break;
    }
  }

  f1 = 0;

  for (int i = posY - 1; i >= 0; i--) {
    if (f1 == 0) {
      if (board[posX][i] == oppPlayer) {
        f1 = 1;
        continue;
      }

      break;
    }

    if (board[posX][i] == 0) {
      break;
    } else if (board[posX][i] == player) {
      legal = 1;

      for (int y = posY; y > i; y--) {
        board[posX][y] = player;
        matrix.fillRect(orient + 2 * posX, 2 * y, 2, 2, getColor(player));
        delay(175);
      }

      break;
    }
  }

  f1 = 0;

  int j = posY - 1;
  for (int i = posX - 1; i >= 0 && j >= 0; i--) {
    if (f1 == 0) {
      if (board[i][j] == oppPlayer) {
        f1 = 1;
        j--;
        continue;
      }

      break;
    }

    if (board[i][j] == 0) {
      break;
    } else if (board[i][j] == player) {
      legal = 1;

      int y = posY;
      for (int x = posX; x > i && y > j; x--) {
        board[x][y] = player;
        matrix.fillRect(orient + 2 * x, 2 * y, 2, 2, getColor(player));
        delay(175);

        y--;
      }
      
      break;
    }

    j--;
  }

  f1 = 0;

  j = posY + 1;
  for (int i = posX - 1; i >= 0 && j <= 7; i--) {
    if (f1 == 0) {
      if (board[i][j] == oppPlayer) {
        f1 = 1;
        j++;
        continue;
      }

      break;
    }

    if (board[i][j] == 0) {
      break;
    } else if (board[i][j] == player) {
      legal = 1;

      int y = posY;
      for (int x = posX; x > i && y < j; x--) {
        board[x][y] = player;
        matrix.fillRect(orient + 2 * x, 2 * y, 2, 2, getColor(player));
        delay(175);

        y++;
      }

      break;
    }

    j++;
  }
  
  f1 = 0;

  j = posY - 1;
  for (int i = posX + 1; i <= 7 && j >= 0; i++) {
    if (f1 == 0) {
      if (board[i][j] == oppPlayer) {
        f1 = 1;
        j--;
        continue;
      }

      break;
    }

    if (board[i][j] == 0) {
      break;
    } else if (board[i][j] == player) {
      legal = 1;

      int y = posY;
      for (int x = posX; x < i && y > j; x++) {
        board[x][y] = player;
        matrix.fillRect(orient + 2 * x, 2 * y, 2, 2, getColor(player));
        delay(175);

        y--;
      }

      break;
    }

    j--;
  }

  f1 = 0;

  j = posY + 1;
  for (int i = posX + 1; i <= 7 && j <= 7; i++) {
    if (f1 == 0) {
      if (board[i][j] == oppPlayer) {
        f1 = 1;
        j++;
        continue;
      }

      break;
    }

    if (board[i][j] == 0) {
      break;
    } else if (board[i][j] == player) {
      legal = 1;

      int y = posY;
      for (int x = posX; x < i && y < j; x++) {
        board[x][y] = player;
        matrix.fillRect(orient + 2 * x, 2 * y, 2, 2, getColor(player));
        delay(175);

        y++;
      }

      break;
    }

    j++;
  }

  if (legal) {
    posX = 0;
    posY = 0;

    player = oppPlayer;

    score1 = 0;
    score2 = 0;

    for (int a = 0; a < 8; a++) {
      for (int b = 0; b < 8; b++) {
        if (board[a][b] == 1) {
          score1++;
        } else if (board[a][b] == 2) {
          score2++;
        }
      }
    }

    sendScore();

    renderSquare(0, 0, cursorColor);
  }

  renderEdges();

  return 0;
}

void process(int a, int b) {
  if (a == 1) {
      // signal to change color
      colorCode = b;
      render();
    } else if (a == 2) {
      // signal to change time limit
      if (b == 0) {
        timeLimit = 0;
      } else if (b == 1) {
        timeLimit = 5;
      } else {
        timeLimit = 30;
      }
    } else if (a == 3) {
      // signal to change board position
      if (b == 0) {
        orient = 0;
        render();
      } else if (b == 1) {
        orient = 8;
        render();
      } else {
        orient = 16;
        render();
      }
    } else if (a == 4) {
      // signal to reset game
      //Serial.write(201);
      reset();
    } else if (a == 5) {
      if (score1 + score2 >= 64) return;

      // process joystick movement
      if (b == 0) {
        if (posY > 0) {
          renderSquare(posX, posY);
          posY--;
          renderSquare(posX, posY, cursorColor);
        }
      } else if (b == 1) {
        if (posX < 7) {
          renderSquare(posX, posY);
          posX++;
          renderSquare(posX, posY, cursorColor);
        }
      } else if (b == 2) {
        if (posY < 7) {
          renderSquare(posX, posY);
          posY++;
          renderSquare(posX, posY, cursorColor);
        }
      } else if (b == 3) {
        if (posX > 0) {
          renderSquare(posX, posY);
          posX--;
          renderSquare(posX, posY, cursorColor);
        }
      }
    } else if (a == 6) {
      if (score1 + score2 >= 64) return;

      int result = checkMove();

      if (result == 0) {
        int r2 = commit();

        if (r2 == 1) {
          Serial.write(9);

          return;
        }
        
        if (score1 + score2 >= 64) {
          if (score1 == score2) {
            Serial.write(7);
            Serial.write(15);
            Serial.write(0);
            Serial.write(0);
            //Serial.println("Game tied...");
            //Serial.println("");
          } else if (score1 > score2) {
            Serial.write(7);
            Serial.write(16);
            Serial.write(0);
            Serial.write(0);
            //Serial.println("Player 1 wins!");
            //Serial.println("");
          } else {
            Serial.write(7);
            Serial.write(17);
            Serial.write(0);
            Serial.write(0);
            //Serial.println("Player 2 wins!");
            //Serial.println("");
          }
        }
      } else if (result == 1) {
        Serial.write(8);
      }
    }
}

// swap function
void swap(int& a, int& b) {
    int temp = a;
    a = b;
    b = temp;
}

// rotate
void rotateMatrix(int matrix[8][8]) {
    // transpose matrix
    for (int i = 0; i < 8; i++) {
        for (int j = i+1; j < 8; j++) {
            swap(matrix[i][j], matrix[j][i]);
        }
    }
    
    // reverse rows
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 4; j++) {
            swap(matrix[i][j], matrix[i][7-j]);
        }
    }
}

// listen for inputs and what not
void loop() {
  while (Serial.available() < 2) {
    delay(50);

    // check button
    if (digitalRead(13) == LOW) {
      // rotate board clockwise

      rotateMatrix(board);
      render();

      delay(1000);
    }
  }

  int a = Serial.read();
  int b = Serial.read();

  process(a, b);
}