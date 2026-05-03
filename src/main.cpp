#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino_BMI270_BMM150.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// -------- FUNCTION PROTOTYPE (IMPORTANT for PlatformIO) --------
void drawPage();

// Buttons
#define BTN_NEXT 2
#define BTN_PREV 3
#define BTN_ACTION 4

int page = 0;

// -------- IMU --------
float ax, ay, az;
float zFiltered = 1.0;

// -------- REP LOGIC --------
int reps = 0;

enum RepState { IDLE, DOWN, UP };
RepState repState = IDLE;

unsigned long lastRepTime = 0;
const int repDelay = 800;

// -------- TIMER --------
unsigned long workoutStart = 0;
bool workoutActive = false;

// -------- BUTTON --------
unsigned long lastPress = 0;

// ---------------- SETUP ----------------
void setup() {
  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PREV, INPUT_PULLUP);
  pinMode(BTN_ACTION, INPUT_PULLUP);

  Serial.begin(115200);

  if (!IMU.begin()) {
    while (1);
  }

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (1);
  }

  drawPage();
}

// ---------------- LOOP ----------------
void loop() {

  // -------- BUTTONS --------
  if (millis() - lastPress > 200) {

    if (digitalRead(BTN_NEXT) == LOW) {
      page = (page + 1) % 3;
      lastPress = millis();
    }

    if (digitalRead(BTN_PREV) == LOW) {
      page = (page - 1 + 3) % 3;
      lastPress = millis();
    }

    if (digitalRead(BTN_ACTION) == LOW) {
      lastPress = millis();

      if (page == 0) {
        reps = 0;
        workoutStart = millis();
        workoutActive = true;
        page = 2;
      }
    }
  }

  // -------- IMU --------
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(ax, ay, az);
    zFiltered = 0.8 * zFiltered + 0.2 * az;
  }

  // -------- REP COUNT --------
  if (workoutActive) {

    unsigned long now = millis();

    switch (repState) {

      case IDLE:
        if (zFiltered < 0.9) repState = DOWN;
        break;

      case DOWN:
        if (zFiltered > 1.15) repState = UP;
        break;

      case UP:
        if (zFiltered > 1.05 && now - lastRepTime > repDelay) {
          reps++;
          lastRepTime = now;
          repState = IDLE;
        }
        break;
    }
  }

  drawPage();
  delay(50);
}

// ---------------- DRAW ----------------
void drawPage() {

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  if (page == 0) {
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("MENU");
    display.println("Press ACTION to start");
  }

  else if (page == 1) {
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("IMU DEBUG");

    display.print("Z: ");
    display.println(zFiltered, 2);
  }

  else if (page == 2) {

    display.setTextSize(2);
    display.setCursor(0, 0);
    display.println("REPS");

    display.setTextSize(3);
    display.setCursor(0, 24);
    display.println(reps);

    display.setTextSize(1);
    display.setCursor(80, 0);

    if (workoutActive) {
      int seconds = (millis() - workoutStart) / 1000;
      display.print(seconds);
      display.print("s");
    }
  }

  display.display();
}