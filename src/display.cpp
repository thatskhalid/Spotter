#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "display.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void initDisplay() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
}

void showReps(int reps) {
  display.clearDisplay();

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("REPS");

  display.setTextSize(3);
  display.setCursor(0, 25);
  display.println(reps);

  display.display();
}

void showText(const char* text) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 20);
  display.println(text);
  display.display();
}

void showTimer(unsigned long timeLeft) {
  display.clearDisplay();

  int seconds = timeLeft / 1000;

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("TIMER");

  display.setTextSize(3);
  display.setCursor(0, 25);
  display.println(seconds);

  display.display();
}