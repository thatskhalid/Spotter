#include "timer.h"
#include "display.h"

static unsigned long startTime = 0;
static int duration = 0;
static bool running = false;

void startTimer(int durationMs) {
  startTime = millis();
  duration = durationMs;
  running = true;
}

bool isTimerRunning() {
  return running;
}

void updateTimer() {
  if (!running) return;

  unsigned long elapsed = millis() - startTime;

  if (elapsed >= duration) {
    showText("DONE");
    running = false;
    return;
  }

  showTimer(duration - elapsed);
}