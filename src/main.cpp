#include <Arduino.h>
#include "imu.h"
#include "display.h"
#include "exercises.h"
#include "timer.h"

enum Mode {
  BENCH,
  SHOULDER,
  CURL,
  TIMER
};

Mode currentMode = BENCH;

float ax, ay, az;

void setup() {
  initIMU();
  initDisplay();

  showText("Ready");
  delay(1000);
}

void loop() {
  readIMU(ax, ay, az);

  switch (currentMode) {

    case BENCH:
      handleBench(az);
      break;

    case SHOULDER:
      handleShoulderPress(az);
      break;

    case CURL:
      handleBicepCurl(ay);
      break;

    case TIMER:
      updateTimer();
      break;
  }

  delay(20);
}