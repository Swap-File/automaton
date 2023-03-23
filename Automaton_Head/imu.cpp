
#include <Arduino.h>
#include "LSM6DS3.h"
#include "Fusion.h"
#include "imu.h"

#define IMU_SAMPLE_RATE (0.01f)  //lie about sample rate for nicer filtering

#define TAP_TIMEOUT 500
#define TAP_SPACING 100
static FusionEuler euler;
static LSM6DS3 myIMU(I2C_MODE, 0x6A);
static FusionAhrs ahrs;

volatile static uint32_t tap_time_previous = 0;
volatile static uint8_t tap_interrupt_counter = 0;

static void tap_isr() {
  uint32_t tap_time = millis();
  if (tap_time - tap_time_previous < TAP_SPACING) {
    return;
  }
  tap_interrupt_counter++;
  tap_time_previous = tap_time;
}

void imu_update(struct cpu_struct *hand_data) {

  if (tap_interrupt_counter > 0 && millis() - tap_time_previous > TAP_TIMEOUT) {
    hand_data->tap_event_counter++;
    hand_data->tap_event = tap_interrupt_counter;
    tap_interrupt_counter = 0;
  }


  const FusionVector gyroscope = { myIMU.readFloatGyroZ(), myIMU.readFloatGyroX(), myIMU.readFloatGyroY() };         // replace this with actual gyroscope data in degrees/s
  const FusionVector accelerometer = { myIMU.readFloatAccelZ(), myIMU.readFloatAccelX(), myIMU.readFloatAccelY() };  // replace this with actual accelerometer data in g
  FusionAhrsUpdateNoMagnetometer(&ahrs, gyroscope, accelerometer, IMU_SAMPLE_RATE);
  euler = FusionQuaternionToEuler(FusionAhrsGetQuaternion(&ahrs));


  hand_data->yaw = constrain(map(euler.angle.yaw + 180, 0, 360, 0, 256), 0, 255);
  hand_data->pitch = constrain(map(euler.angle.pitch + 180, 0, 360, 0, 256), 0, 255);
  hand_data->roll = constrain(map(euler.angle.roll, 0, 360, 0, 256), 0, 255);

  myIMU.readRegister(&(hand_data->step_count), LSM6DS3_ACC_GYRO_STEP_COUNTER_L);
}

void imu_init(void) {

  FusionAhrsInitialise(&ahrs);

  // Set AHRS algorithm settings
  const FusionAhrsSettings settings = {
    .gain = 0.5f,
    .accelerationRejection = 10.0f,
    .magneticRejection = 0.0f,
    .rejectionTimeout = 5,
  };
  FusionAhrsSetSettings(&ahrs, &settings);

  if (myIMU.begin() != 0) {
    Serial.println("myIMU error");
  } else {
    Serial.println("myIMU OK!");
  }

  pinMode(PIN_LSM6DS3TR_C_INT1, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_LSM6DS3TR_C_INT1), tap_isr, RISING);

  myIMU.writeRegister(LSM6DS3_ACC_GYRO_TAP_CFG1, 0x8E);          // INTERRUPTS_ENABLE, SLOPE_FDS
  myIMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL10_C, 0x3C);          //enable pedometer
  myIMU.writeRegister(LSM6DS3_ACC_GYRO_TAP_THS_6D, 0b11000001);  //tap threshold
  //myIMU.writeRegister(LSM6DS3_ACC_GYRO_INT_DUR2, 0x7F); //tap duraction
  myIMU.writeRegister(LSM6DS3_ACC_GYRO_MD1_CFG, 0b01000000);  //single tap route to int
}
