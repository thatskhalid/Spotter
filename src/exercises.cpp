#include "exercises.h"
#include "display.h"

static int repCount = 0;
static unsigned long lastRepTime = 0;
static int minRepDelay = 1000;

static float zFiltered = 0;
static float yFiltered = 0;

enum State {
  IDLE,
  GOING_DOWN,
  GOING_UP
};

static State state = IDLE;

void resetReps() {
  repCount = 0;
  state = IDLE;
}

int getReps() {
  return repCount;
}

// ---------- BENCH ----------
void handleBench(float az) {
  zFiltered = 0.8 * zFiltered + 0.2 * az;
  unsigned long now = millis();

  switch (state) {
    case IDLE:
      if (zFiltered < 0.9) state = GOING_DOWN;
      break;

    case GOING_DOWN:
      if (zFiltered > 1.15) state = GOING_UP;
      break;

    case GOING_UP:
      if (zFiltered > 1.05) {
        if (now - lastRepTime > minRepDelay) {
          repCount++;
          lastRepTime = now;
          showReps(repCount);
        }
        state = IDLE;
      }
      break;
  }
}

// ---------- SHOULDER PRESS ----------
void handleShoulderPress(float az) {
  zFiltered = 0.8 * zFiltered + 0.2 * az;
  unsigned long now = millis();

  switch (state) {
    case IDLE:
      if (zFiltered < 0.95) state = GOING_DOWN;
      break;

    case GOING_DOWN:
      if (zFiltered > 1.2) state = GOING_UP;
      break;

    case GOING_UP:
      if (zFiltered > 1.1) {
        if (now - lastRepTime > minRepDelay) {
          repCount++;
          lastRepTime = now;
          showReps(repCount);
        }
        state = IDLE;
      }
      break;
  }
}

// ---------- BICEP CURL ----------
void handleBicepCurl(float ay) {
  yFiltered = 0.8 * yFiltered + 0.2 * ay;
  unsigned long now = millis();

  switch (state) {
    case IDLE:
      if (yFiltered > 0.5) state = GOING_UP;
      break;

    case GOING_UP:
      if (yFiltered < -0.3) state = GOING_DOWN;
      break;

    case GOING_DOWN:
      if (yFiltered > 0.2) {
        if (now - lastRepTime > minRepDelay) {
          repCount++;
          lastRepTime = now;
          showReps(repCount);
        }
        state = IDLE;
      }
      break;
  }
}