#include <Wire.h> 

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino_LSM9DS1.h>  // 🔁 Changed IMU library

int repCount = 0;
float accelZFiltered = 0;

enum State {
  IDLE,
  GOING_DOWN,
  GOING_UP
};

State currentState = IDLE;

unsigned long lastRepTime = 0;
int minRepDelay = 800; // milliseconds

// OLED setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_RESET);

void setup() {
  Serial.begin(9600);
  Wire.begin();

  // 🔁 Initialize LSM9DS1
  if (!IMU.begin()) {
    Serial.println("IMU connection failed!");
    while (1);
  } else {
    Serial.println("IMU connected!");
  }

  // OLED init
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED failed!");
    while (1);
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(10, 10);
  display.println("Ready!");
  display.display();
  delay(1000);
}

void loop() {
  float ax, ay, az;

  // 🔁 Read acceleration from LSM9DS1
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(ax, ay, az);
  }

  // Same scaling assumption (already in g's for this lib)
  float accelZ = az;

  // Low-pass filter
  accelZFiltered = 0.8 * accelZFiltered + 0.2 * accelZ;

  unsigned long currentTime = millis();

  switch (currentState) {

    case IDLE:
      if (accelZFiltered < 0.9) {
        currentState = GOING_DOWN;
      }
      break;

    case GOING_DOWN:
      if (accelZFiltered > 1.15) {
        currentState = GOING_UP;
      }
      break;

    case GOING_UP:
      if (accelZFiltered > 1.05) {
        if (currentTime - lastRepTime > minRepDelay) {
          repCount++;
          lastRepTime = currentTime;

          // 🔥 OLED UPDATE
          display.clearDisplay();
          display.setTextSize(2);

          display.setCursor(0, 0);
          display.println("REPS");

          display.setTextSize(3);
          display.setCursor(0, 25);
          display.println(repCount);

          display.display();
        }
        currentState = IDLE;
      }
      break;
  }

  delay(20);
}