#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino_BMI270_BMM150.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// =====================================================
// BUTTONS
// =====================================================
#define BTN_UP       2
#define BTN_DOWN     3
#define BTN_SELECT   4
#define BTN_BACK     5

// =====================================================
// UI STATES
// =====================================================
enum UIState {
  WELCOME,
  MAIN_MENU,
  SPOTTER_MENU,

  WORKOUT_READY,
  WORKOUT_ACTIVE,

  SET_COMPLETE,

  REST,

  DEBUG
};

UIState state = WELCOME;

// =====================================================
// MENUS
// =====================================================
int mainIndex = 0;
const char* mainMenu[] = {
  "Spotter",
  "Debug"
};

int exerciseIndex = 0;
const char* exercises[] = {
  "Press",
  "Curl",
  "Pulldown"
};

int setMenuIndex = 0;

const char* setMenu[] = {
  "Rest Timer",
  "New Set",
  "Clear Reps"
};

// =====================================================
// BUTTON EDGE DETECTION
// =====================================================
bool lastButtonState[20];

bool pressed(int pin) {

  bool current = digitalRead(pin);

  if (current == LOW && lastButtonState[pin] == HIGH) {
    lastButtonState[pin] = LOW;
    delay(10);
    return true;
  }

  if (current == HIGH) {
    lastButtonState[pin] = HIGH;
  }

  return false;
}

// =====================================================
// IMU
// =====================================================
float ax, ay, az;
float zFiltered = 1.0;

// =====================================================
// WORKOUT
// =====================================================
bool counting = false;

int reps = 0;
int currentSet = 1;

enum RepState {
  IDLE,
  DOWN,
  UP
};

RepState repState = IDLE;

unsigned long lastRepTime = 0;
const int repDelay = 800;

// =====================================================
// REST TIMER
// =====================================================
unsigned long restStart = 0;

const unsigned long restDuration = 30000;

bool restFinished = false;

// =====================================================
// PROTOTYPES
// =====================================================
void drawUI();
void handleButtons();
void updateIMU();
void updateReps();

// =====================================================
// SETUP
// =====================================================
void setup() {

  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);

  Serial.begin(115200);

  if (!IMU.begin()) {
    while (1);
  }

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (1);
  }

  display.clearDisplay();
  display.display();

  // Initialize button states
  for (int i = 0; i < 20; i++) {
    lastButtonState[i] = HIGH;
  }
}

// =====================================================
// LOOP
// =====================================================
void loop() {

  handleButtons();

  updateIMU();

  // Active workout rep counting
  if (state == WORKOUT_ACTIVE && counting) {
    updateReps();
  }

  // Rest timer logic
  if (state == REST) {

    unsigned long elapsed = millis() - restStart;

    if (elapsed >= restDuration) {
      restFinished = true;
    }
  }

  drawUI();

  delay(40);
}

// =====================================================
// BUTTON HANDLING
// =====================================================
void handleButtons() {

  // =================================================
  // UP BUTTON
  // =================================================
  if (pressed(BTN_UP)) {

    if (state == MAIN_MENU) {

      mainIndex--;

      if (mainIndex < 0) mainIndex = 1;
    }

    else if (state == SPOTTER_MENU) {

      exerciseIndex--;

      if (exerciseIndex < 0) exerciseIndex = 2;
    }

    else if (state == SET_COMPLETE) {

      setMenuIndex--;

      if (setMenuIndex < 0) setMenuIndex = 2;
    }
  }

  // =================================================
  // DOWN BUTTON
  // =================================================
  if (pressed(BTN_DOWN)) {

    if (state == MAIN_MENU) {

      mainIndex++;

      if (mainIndex > 1) mainIndex = 0;
    }

    else if (state == SPOTTER_MENU) {

      exerciseIndex++;

      if (exerciseIndex > 2) exerciseIndex = 0;
    }

    else if (state == SET_COMPLETE) {

      setMenuIndex++;

      if (setMenuIndex > 2) setMenuIndex = 0;
    }
  }

  // =================================================
  // SELECT BUTTON
  // =================================================
  if (pressed(BTN_SELECT)) {

    switch (state) {

      // ---------------------------------------------
      case WELCOME:
        state = MAIN_MENU;
        break;

      // ---------------------------------------------
      case MAIN_MENU:

        if (mainIndex == 0) {
          state = SPOTTER_MENU;
        }

        else {
          state = DEBUG;
        }

        break;

      // ---------------------------------------------
      case SPOTTER_MENU:

        reps = 0;
        currentSet = 1;

        state = WORKOUT_READY;

        break;

      // ---------------------------------------------
      case WORKOUT_READY:

        reps = 0;
        counting = true;

        repState = IDLE;

        state = WORKOUT_ACTIVE;

        break;

      // ---------------------------------------------
      case SET_COMPLETE:

        // REST TIMER
        if (setMenuIndex == 0) {

          restStart = millis();

          restFinished = false;

          state = REST;
        }

        // NEW SET
        else if (setMenuIndex == 1) {

          reps = 0;

          currentSet++;

          repState = IDLE;

          state = WORKOUT_READY;
        }

        // CLEAR REPS
        else if (setMenuIndex == 2) {

          reps = 0;
        }

        break;

      // ---------------------------------------------
      case REST:

        // Rest finished -> start next set
        if (restFinished) {

          reps = 0;

          currentSet++;

          repState = IDLE;

          state = WORKOUT_READY;
        }

        break;

      // ---------------------------------------------
      case DEBUG:
        state = MAIN_MENU;
        break;
    }
  }

  // =================================================
  // BACK BUTTON
  // =================================================
  if (pressed(BTN_BACK)) {

    switch (state) {

      // ---------------------------------------------
      case MAIN_MENU:
        state = WELCOME;
        break;

      // ---------------------------------------------
      case SPOTTER_MENU:
        state = MAIN_MENU;
        break;

      // ---------------------------------------------
      case WORKOUT_READY:
        state = SPOTTER_MENU;
        break;

      // ---------------------------------------------
      case WORKOUT_ACTIVE:

        counting = false;

        state = SET_COMPLETE;

        break;

      // ---------------------------------------------
      case SET_COMPLETE:

        state = WORKOUT_READY;

        break;

      // ---------------------------------------------
      case REST:

        // Skip rest early
        restFinished = false;

        state = WORKOUT_READY;

        break;

      // ---------------------------------------------
      case DEBUG:
        state = MAIN_MENU;
        break;
    }
  }
}

// =====================================================
// IMU UPDATE
// =====================================================
void updateIMU() {

  if (IMU.accelerationAvailable()) {

    IMU.readAcceleration(ax, ay, az);

    zFiltered = 0.8 * zFiltered + 0.2 * az;
  }
}

// =====================================================
// REP LOGIC
// =====================================================
void updateReps() {

  unsigned long now = millis();

  switch (exerciseIndex) {

    // =================================================
    // PRESS
    // =================================================
    case 0:

      if (repState == IDLE && zFiltered < 0.9) {

        repState = DOWN;
      }

      else if (repState == DOWN && zFiltered > 1.15) {

        repState = UP;
      }

      else if (repState == UP &&
               zFiltered > 1.05 &&
               now - lastRepTime > repDelay) {

        reps++;

        lastRepTime = now;

        repState = IDLE;
      }

      break;

    // =================================================
    // CURL
    // =================================================
    case 1:

      if (repState == IDLE && zFiltered > 1.1) {

        repState = UP;
      }

      else if (repState == UP &&
               zFiltered < 0.9 &&
               now - lastRepTime > repDelay) {

        reps++;

        lastRepTime = now;

        repState = IDLE;
      }

      break;

    // =================================================
    // PULLDOWN
    // =================================================
    case 2:

      if (repState == IDLE && zFiltered < 0.95) {

        repState = DOWN;
      }

      else if (repState == DOWN &&
               zFiltered > 1.1 &&
               now - lastRepTime > repDelay) {

        reps++;

        lastRepTime = now;

        repState = IDLE;
      }

      break;
  }
}

// =====================================================
// UI DRAWING
// =====================================================
void drawUI() {

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  // =================================================
  // WELCOME
  // =================================================
  if (state == WELCOME) {

    display.setTextSize(2);

    display.setCursor(10, 20);

    display.println("SPOTTER");

    display.setTextSize(1);

    display.setCursor(20, 50);

    display.println("SELECT to start");
  }

  // =================================================
  // MAIN MENU
  // =================================================
  else if (state == MAIN_MENU) {

    display.setTextSize(1);

    display.setCursor(0, 0);
    display.println("MAIN MENU");

    for (int i = 0; i < 2; i++) {

      display.setCursor(0, 18 + (i * 12));

      if (i == mainIndex) display.print("> ");
      else display.print("  ");

      display.println(mainMenu[i]);
    }
  }

  // =================================================
  // SPOTTER MENU
  // =================================================
  else if (state == SPOTTER_MENU) {

    display.setTextSize(1);

    display.setCursor(0, 0);

    display.println("SELECT EXERCISE");

    for (int i = 0; i < 3; i++) {

      display.setCursor(0, 18 + (i * 12));

      if (i == exerciseIndex) display.print("> ");
      else display.print("  ");

      display.println(exercises[i]);
    }
  }

  // =================================================
  // WORKOUT READY
  // =================================================
  else if (state == WORKOUT_READY) {

    display.setTextSize(1);

    display.setCursor(0, 0);
    display.println(exercises[exerciseIndex]);

    display.setCursor(0, 12);
    display.print("Set ");
    display.println(currentSet);

    display.setCursor(0, 24);
    display.print("Reps: ");
    display.println(reps);

    display.setCursor(0, 50);
    display.println("SELECT = Start");
  }

  // =================================================
  // WORKOUT ACTIVE
  // =================================================
  else if (state == WORKOUT_ACTIVE) {

    display.setTextSize(1);

    display.setCursor(0, 0);
    display.println(exercises[exerciseIndex]);

    display.setCursor(80, 0);
    display.println("RUN");

    display.setCursor(0, 10);
    display.print("Set ");
    display.println(currentSet);

    display.setTextSize(4);

    display.setCursor(30, 22);

    display.println(reps);

    display.setTextSize(1);

    display.setCursor(0, 55);

    display.println("BACK = End Set");
  }

  // =================================================
  // SET COMPLETE
  // =================================================
  else if (state == SET_COMPLETE) {

    display.setTextSize(1);

    display.setCursor(0, 0);

    display.println("SET COMPLETE");

    display.setCursor(0, 10);

    display.print("Reps: ");
    display.println(reps);

    for (int i = 0; i < 3; i++) {

      display.setCursor(0, 26 + (i * 10));

      if (i == setMenuIndex) display.print("> ");
      else display.print("  ");

      display.println(setMenu[i]);
    }
  }

  // =================================================
  // REST
  // =================================================
  else if (state == REST) {

    display.setTextSize(1);

    display.setCursor(0, 0);

    display.println("REST");

    if (!restFinished) {

      int remaining =
        (restDuration - (millis() - restStart)) / 1000;

      if (remaining < 0) remaining = 0;

      display.setTextSize(4);

      display.setCursor(35, 20);

      display.println(remaining);

      display.setTextSize(1);

      display.setCursor(0, 55);

      display.println("BACK = Skip");
    }

    else {

      display.setTextSize(2);

      display.setCursor(5, 15);

      display.println("REST OVER!");

      display.setTextSize(1);

      display.setCursor(10, 50);

      display.println("SELECT = Next Set");
    }
  }

  // =================================================
  // DEBUG
  // =================================================
  else if (state == DEBUG) {

    display.setTextSize(1);

    display.setCursor(0, 0);

    display.println("DEBUG");

    display.setCursor(0, 15);

    display.print("Z: ");
    display.println(zFiltered, 2);

    display.setCursor(0, 30);

    display.print("Reps: ");
    display.println(reps);

    display.setCursor(0, 45);

    display.print("State: ");
    display.println(state);
  }

  display.display();
}