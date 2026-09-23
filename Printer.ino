#include "Payload.h"
#include <avr/pgmspace.h>

// Types ------------------------------------------------------
typedef struct {
  uint16_t x;
  uint16_t y;
} Point;


// PINS -------------------------------------------------------

constexpr int UP_PIN = 2;
constexpr int DOWN_PIN = 3;
constexpr int LEFT_PIN = 4;
constexpr int RIGHT_PIN = 5;

constexpr int A_PIN = 6;
constexpr int B_PIN = 7;
constexpr int X_PIN = 8;
constexpr int Y_PIN = 9;

constexpr int R_PIN = 10;
constexpr int ZR_PIN = 11;

constexpr int L_PIN = 12;
constexpr int ZL_PIN = A1;

constexpr int BUTTON_PIN = A0;


// DEFAULTS ---------------------------------------------------
constexpr int CANVAS_WIDTH  = 320;
constexpr int CANVAS_HEIGHT = 120;

constexpr int HOLD_BEFORE_TIME = 30;
constexpr int HOLD_AFTER_TIME = 30;

constexpr int HORIZONTAL_CHUNKS = 40;
constexpr int VERTICAL_CHUNKS = 15;

// HELPER METHODS ---------------------------------------------

void pressButton(int pinNum, int holdTime, int afterWaitTime) {
  digitalWrite(pinNum, HIGH);
  delay(holdTime);
  digitalWrite(pinNum, LOW);
  delay(afterWaitTime); 
}

void pressMultiple(int pinOne, int pinTwo, int holdTime, int afterWaitTime) {
  digitalWrite(pinOne, HIGH);
  digitalWrite(pinTwo, HIGH);
  delay(holdTime);
  digitalWrite(pinOne, LOW);
  digitalWrite(pinTwo, LOW);
  delay(afterWaitTime); 
}

bool startTriggered() {
  return digitalRead(BUTTON_PIN) == LOW;
}

void blinkForever() {
  while(true) {
    pressButton(LED_BUILTIN, 500, 500);
  }
}


// PRINTING METHODS -------------------------------------------
bool isLastStep(int dx, int dy) {
    return (abs(dx) <= 1 && abs(dy) <= 1);
}

void printPixel(Point *location, Point *target) {
  int dx = target->x - location->x;
  int dy = target->y - location->y;

  // Navigate to the target corrdinates using the shortest path.
  while (dx != 0 || dy != 0) {
      if (isLastStep(dx, dy)) {
        digitalWrite(A_PIN, HIGH);
      }

      if (dx > 0 && dy > 0) {
          pressMultiple(RIGHT_PIN, DOWN_PIN, HOLD_BEFORE_TIME, HOLD_AFTER_TIME);
          dx--; dy--;
      } else if (dx > 0 && dy < 0) {
          pressMultiple(RIGHT_PIN, UP_PIN, HOLD_BEFORE_TIME, HOLD_AFTER_TIME);
          dx--; dy++;
      } else if (dx < 0 && dy > 0) {
          pressMultiple(LEFT_PIN, DOWN_PIN, HOLD_BEFORE_TIME, HOLD_AFTER_TIME);
          dx++; dy--;
      } else if (dx < 0 && dy < 0) {
          pressMultiple(LEFT_PIN, UP_PIN, HOLD_BEFORE_TIME, HOLD_AFTER_TIME);
          dx++; dy++;
      } else if (dx > 0) {
          pressButton(RIGHT_PIN, HOLD_BEFORE_TIME, HOLD_AFTER_TIME);
          dx--;
      } else if (dx < 0) {
          pressButton(LEFT_PIN, HOLD_BEFORE_TIME, HOLD_AFTER_TIME);
          dx++;
      } else if (dy > 0) {
          pressButton(DOWN_PIN, HOLD_BEFORE_TIME, HOLD_AFTER_TIME);
          dy--;
      } else if (dy < 0) {
          pressButton(UP_PIN, HOLD_BEFORE_TIME, HOLD_AFTER_TIME);
          dy++;
      }
    }

    digitalWrite(A_PIN, LOW);

    location->x = target->x;
    location->y = target->y;
}

bool isChunkEmpty(uint8_t (*buffer)[8]) {
    for (int i = 0; i < 8; i++) {
      for (int j = 0; j < 8; j++) {
        if (buffer[i][j] != 0) {
          return false;
        }
      }
    }

  return true;
}

int dist(Point a, Point b) {
  return max(abs(a.x - b.x), abs(a.y - b.y));
}

void processChunk(Point *currLocation, Point *chunkLocation, uint8_t (*buffer)[8]) {
  if (isChunkEmpty(buffer)) {
    return;
  }

  // TOP LEFT     - LtR -> Down  Col: 0-8 Row: 0-8
  // TOP RIGHT    - RtL -> Down  Col: 8-0 Row: 0-8
  // BOTTOM LEFT  - LtR -> Up    Col: 0-8 Row: 8-0
  // BOTTOM RIGHT - RtL -> Up    Col: 8-0 Row: 8-0

  Point corners[4] = {
    {chunkLocation->x,     chunkLocation->y},
    {chunkLocation->x + 7, chunkLocation->y},
    {chunkLocation->x,     chunkLocation->y + 7},
    {chunkLocation->x + 7, chunkLocation->y + 7}
  };

  // Find the closest corner.
  int bestIdx = 0;
  int bestDist = dist(*currLocation, corners[0]);

  for (int i = 1; i < 4; i++) {
    int d = dist(*currLocation, corners[i]);
    if (d < bestDist) {
      bestDist = d;
      bestIdx = i;
    }
  }

  bool isTop   = (bestIdx == 0 || bestIdx == 1);
  bool isLeft  = (bestIdx == 0 || bestIdx == 2);

  int yStart = !isTop ? 0 : 7;
  int yEnd   = !isTop ? 8 : -1;
  int yStep  = !isTop ? 1 : -1;

  bool leftToRight = isLeft;

  for (int y = yStart; y != yEnd; y += yStep) {

    int xStart = !leftToRight ? 0 : 7;
    int xEnd   = !leftToRight ? 8 : -1;
    int xStep  = !leftToRight ? 1 : -1;

    for (int x = xStart; x != xEnd; x += xStep) {
      Point currPixel = {
        chunkLocation->x + x,
        chunkLocation->y + y
      };

      if (buffer[y][x] != 0) {
        printPixel(currLocation, &currPixel);
      }
    }

    leftToRight = !leftToRight;
  }
}

void run(uint8_t *pixelInfo) {
  Point location = {0, 0};
  uint8_t currBuffer[8][8];
  
  for (int column = 0; column < HORIZONTAL_CHUNKS; column++) {      
      for (int row = 0; row < VERTICAL_CHUNKS; row++) {
        int actualRow = (column % 2 == 0) ? row : (VERTICAL_CHUNKS - 1 - row);
        
        for (int subRow = 0; subRow < 8; subRow++) {
          int canvasRow = actualRow * 8 + subRow;
          uint8_t byteLine = pgm_read_byte(pixelInfo + canvasRow * HORIZONTAL_CHUNKS + column);
          for (int bit = 0; bit < 8; bit++) {
            currBuffer[subRow][bit] = (byteLine >> (7 - bit)) & 1;
          }
        }

        // Navigate to and print the chunk.
        Point chunkLocation = { column * 8, actualRow * 8 };
        processChunk(&location, &chunkLocation, currBuffer);
      }
  }
}


void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(UP_PIN, OUTPUT);
  pinMode(DOWN_PIN, OUTPUT);
  pinMode(LEFT_PIN, OUTPUT);
  pinMode(RIGHT_PIN, OUTPUT);
  pinMode(A_PIN, OUTPUT);
  pinMode(B_PIN, OUTPUT);
  pinMode(X_PIN, OUTPUT);
  pinMode(Y_PIN, OUTPUT);
  pinMode(R_PIN, OUTPUT);
  pinMode(ZR_PIN, OUTPUT);
  pinMode(L_PIN, OUTPUT);
  pinMode(ZL_PIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_BUILTIN, LOW);
  
  if (startTriggered()) {
    digitalWrite(LED_BUILTIN, HIGH);
    
    run(PAYLOAD);
    blinkForever();
  }
}