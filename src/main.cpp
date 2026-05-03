#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino_BMI270_BMM150.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// -------- BUTTONS --------
#define BTN_NEXT 2
#define BTN_PREV 3
#define BTN_ACTION 4

// -------- UI STATES --------
enum UIState {
  WELCOME,
  MAIN_MENU,
  SPOTTER_MENU,
  WORKOUT,
  REST,
  DEBUG
};

UIState state = WELCOME;

// -------- MENU --------
int mainIndex = 0;
const char* mainMenu[] = {"Spotter", "Debug"};

int exerciseIndex = 0;
const char* exercises[] = {"Press", "Curl", "Pulldown"};

// -------- BUTTON CONTROL --------
unsigned long lastPress = 0;
const int debounce = 180;

// -------- IMU --------
float ax, ay, az;
float zFiltered = 1.0;

// -------- WORKOUT --------
bool counting = false;
int reps = 0;

enum RepState { IDLE, DOWN, UP };
RepState repState = IDLE;

unsigned long lastRepTime = 0;
const int repDelay = 800;

// -------- REST --------
unsigned long restStart = 0;
bool restActive = false;

// -------- PROTOTYPE --------
void drawUI();
void handleButtons();
void updateIMU();
void updateReps();

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
}

// ---------------- LOOP ----------------
void loop() {
  handleButtons();
  updateIMU();

  if (state == WORKOUT && counting) {
    updateReps();
  }

  drawUI();
  delay(40);
}

// ---------------- BUTTONS ----------------
void handleButtons() {

  if (millis() - lastPress < debounce) return;

  if (digitalRead(BTN_NEXT) == LOW) {
    lastPress = millis();

    if (state == MAIN_MENU) mainIndex = (mainIndex + 1) % 2;
    else if (state == SPOTTER_MENU) exerciseIndex = (exerciseIndex + 1) % 3;
  }

  if (digitalRead(BTN_PREV) == LOW) {
    lastPress = millis();

    if (state == MAIN_MENU) mainIndex = (mainIndex + 1) % 2;
    else if (state == SPOTTER_MENU) exerciseIndex = (exerciseIndex + 2) % 3;
  }

  if (digitalRead(BTN_ACTION) == LOW) {
    lastPress = millis();

    switch (state) {

      case WELCOME:
        state = MAIN_MENU;
        break;

      case MAIN_MENU:
        if (mainIndex == 0) state = SPOTTER_MENU;
        else state = DEBUG;
        break;

      case SPOTTER_MENU:
        reps = 0;
        counting = false;
        state = WORKOUT;
        break;

      case WORKOUT:
        if (!counting) counting = true;
        else counting = false; // stop count
        break;

      case REST:
        state = MAIN_MENU;
        break;

      case DEBUG:
        state = MAIN_MENU;
        break;
    }
  }
}

// ---------------- IMU ----------------
void updateIMU() {
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(ax, ay, az);
    zFiltered = 0.8 * zFiltered + 0.2 * az;
  }
}

// ---------------- REP LOGIC ----------------
void updateReps() {

  unsigned long now = millis();

  switch (exerciseIndex) {

    // -------- PRESS --------
    case 0:
      if (repState == IDLE && zFiltered < 0.9) repState = DOWN;
      else if (repState == DOWN && zFiltered > 1.15) repState = UP;
      else if (repState == UP && zFiltered > 1.05 && now - lastRepTime > repDelay) {
        reps++;
        lastRepTime = now;
        repState = IDLE;
      }
      break;

    // -------- CURL --------
    case 1:
      if (repState == IDLE && zFiltered > 1.1) repState = UP;
      else if (repState == UP && zFiltered < 0.9 && now - lastRepTime > repDelay) {
        reps++;
        lastRepTime = now;
        repState = IDLE;
      }
      break;

    // -------- PULLDOWN --------
    case 2:
      if (repState == IDLE && zFiltered < 0.95) repState = DOWN;
      else if (repState == DOWN && zFiltered > 1.1 && now - lastRepTime > repDelay) {
        reps++;
        lastRepTime = now;
        repState = IDLE;
      }
      break;
  }
}

// ---------------- UI ----------------
void drawUI() {

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // ===== WELCOME =====
  if (state == WELCOME) {
    display.setTextSize(2);
    display.setCursor(10, 20);
    display.println("SPOTTER");
  }

  // ===== MAIN MENU =====
  else if (state == MAIN_MENU) {
    display.setTextSize(1);

    for (int i = 0; i < 2; i++) {
      display.setCursor(0, i * 12);
      if (i == mainIndex) display.print("> ");
      else display.print("  ");
      display.println(mainMenu[i]);
    }
  }

  // ===== SPOTTER MENU =====
  else if (state == SPOTTER_MENU) {
    display.setTextSize(1);

    for (int i = 0; i < 3; i++) {
      display.setCursor(0, i * 12);
      if (i == exerciseIndex) display.print("> ");
      else display.print("  ");
      display.println(exercises[i]);
    }
  }

  // ===== WORKOUT =====
  else if (state == WORKOUT) {

    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(exercises[exerciseIndex]);

    display.setTextSize(3);
    display.setCursor(0, 20);
    display.println(reps);

    display.setTextSize(1);
    display.setCursor(80, 0);

    if (counting) display.println("RUN");
    else display.println("STOP");

    display.setCursor(0, 55);
    display.println("Press: start/stop");
  }

  // ===== DEBUG =====
  else if (state == DEBUG) {
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("DEBUG");

    display.print("Z:");
    display.println(zFiltered, 2);
  }

  display.display();
}