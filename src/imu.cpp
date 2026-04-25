#include <Arduino_BMI270_BMM150.h>
#include "imu.h"

void initIMU() {
  if (!IMU.begin()) {
    while (1);
  }
}

void readIMU(float &ax, float &ay, float &az) {
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(ax, ay, az);
  }
}