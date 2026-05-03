#include <Wire.h>
#include <Arduino_BMI270_BMM150.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// -------------------- DISPLAY --------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// -------------------- BUTTONS --------------------
#define BTN_UP 2
#define BTN_DOWN 3
#define BTN_SELECT 4

// -------------------- VIBRATION --------------------
#define VIB_PIN 5

// -------------------- STATES --------------------
enum SystemState {
  MENU,
  GRACE,
  WORKOUT,
  REST
};

SystemState currentSystemState = MENU;

// -------------------- MENU --------------------
const char* exercises[] = {
  "Bench Press",
  "Shoulder Press",
  "Bicep Curl"
};

int selectedExercise = 0;

// -------------------- REP LOGIC --------------------
int repCount = 0;
float accelZFiltered = 0;

enum RepState {
  IDLE,
  GOING_DOWN,
  GOING_UP
};

RepState repState = IDLE;

unsigned long lastRepTime = 0;
int minRepDelay = 1000;

// -------------------- TIMERS --------------------
unsigned long graceStart = 0;
unsigned long restStart = 0;

void handleButtons();
void showMenu();
void handleGrace();
void handleWorkout();
void handleRest();

// -------------------- SETUP --------------------
void setup() {
  Serial.begin(9600);
  Wire.begin();

  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);

  pinMode(VIB_PIN, OUTPUT);

  if (!IMU.begin()) {
    Serial.println("IMU failed");
    while (1);
  }

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED failed");
    while (1);
  }

  display.clearDisplay();
}

// -------------------- LOOP --------------------
void loop() {

  handleButtons();

  switch (currentSystemState) {

    case MENU:
      showMenu();
      break;

    case GRACE:
      handleGrace();
      break;

    case WORKOUT:
      handleWorkout();
      break;

    case REST:
      handleRest();
      break;
  }
}

void handleButtons() {

  if (digitalRead(BTN_UP) == LOW) {
    selectedExercise--;
    if (selectedExercise < 0) selectedExercise = 2;
    delay(200);
  }

  if (digitalRead(BTN_DOWN) == LOW) {
    selectedExercise++;
    if (selectedExercise > 2) selectedExercise = 0;
    delay(200);
  }

  if (digitalRead(BTN_SELECT) == LOW) {
    if (currentSystemState == MENU) {
      currentSystemState = GRACE;
      graceStart = millis();
      repCount = 0;
    }
    else if (currentSystemState == WORKOUT) {
      currentSystemState = REST;
      restStart = millis();
    }
    delay(200);
  }
}

void showMenu() {
  display.clearDisplay();
  display.setTextSize(1);

  for (int i = 0; i < 3; i++) {
    display.setCursor(0, i * 10);

    if (i == selectedExercise) {
      display.print("> ");
    } else {
      display.print("  ");
    }

    display.println(exercises[i]);
  }

  display.display();
}


void handleGrace() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 20);
  display.println("GET READY");
  display.display();

  if (millis() - graceStart > 5000) {
    digitalWrite(VIB_PIN, HIGH);
    delay(300);
    digitalWrite(VIB_PIN, LOW);

    currentSystemState = WORKOUT;
  }
}

void handleWorkout() {

  float ax, ay, az;

  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(ax, ay, az);
  }

  float accelZ = az;
  accelZFiltered = 0.8 * accelZFiltered + 0.2 * accelZ;

  unsigned long currentTime = millis();

  switch (repState) {

    case IDLE:
      if (accelZFiltered < 0.9) {
        repState = GOING_DOWN;
      }
      break;

    case GOING_DOWN:
      if (accelZFiltered > 1.15) {
        repState = GOING_UP;
      }
      break;

    case GOING_UP:
      if (accelZFiltered > 1.05) {
        if (currentTime - lastRepTime > minRepDelay) {
          repCount++;
          lastRepTime = currentTime;
        }
        repState = IDLE;
      }
      break;
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("REPS");

  display.setTextSize(3);
  display.setCursor(0, 25);
  display.println(repCount);

  display.display();
}


void handleRest() {

  int remaining = 120000 - (millis() - restStart);

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("REST");

  display.setTextSize(2);
  display.setCursor(0, 30);
  display.println(remaining / 1000);

  display.display();

  if (remaining <= 0) {
    digitalWrite(VIB_PIN, HIGH);
    delay(500);
    digitalWrite(VIB_PIN, LOW);

    currentSystemState = MENU;
  }
}


