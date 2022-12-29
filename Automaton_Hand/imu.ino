
#define IMU_SAMPLE_RATE (0.01f)  //lie about sample rate for nice filtering
#define int1Pin PIN_LSM6DS3TR_C_INT1

volatile static uint32_t last_tap_time = 0;
volatile uint8_t interruptCount = 0;

void int1ISR() {
  uint32_t tap_time = millis();
  if (tap_time - last_tap_time < 100) {  // too soon
    return;
  }
  interruptCount++;
  last_tap_time = tap_time;
}

void imu_isr_check(void) {
  if (interruptCount > 0 && millis() - last_tap_time > 500) {
    tap_event_count++;
    tap_event_last = interruptCount;
    interruptCount = 0;
  }
}

void update_imu(void) {
  const FusionVector gyroscope = { myIMU.readFloatGyroX(), myIMU.readFloatGyroY(), myIMU.readFloatGyroZ() };         // replace this with actual gyroscope data in degrees/s
  const FusionVector accelerometer = { myIMU.readFloatAccelX(), myIMU.readFloatAccelY(), myIMU.readFloatAccelZ() };  // replace this with actual accelerometer data in g
  FusionAhrsUpdateNoMagnetometer(&ahrs, gyroscope, accelerometer, IMU_SAMPLE_RATE);
  euler = FusionQuaternionToEuler(FusionAhrsGetQuaternion(&ahrs));
  myIMU.readRegister(&stepCount, LSM6DS3_ACC_GYRO_STEP_COUNTER_L);
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

  pinMode(int1Pin, INPUT);
  attachInterrupt(digitalPinToInterrupt(int1Pin), int1ISR, RISING);

  myIMU.writeRegister(LSM6DS3_ACC_GYRO_TAP_CFG1, 0x8E);    // INTERRUPTS_ENABLE, SLOPE_FDS
  myIMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL10_C, 0x3C);    //enable pedometer
  myIMU.writeRegister(LSM6DS3_ACC_GYRO_TAP_THS_6D, 0x8C);  //tap threshold
  //myIMU.writeRegister(LSM6DS3_ACC_GYRO_INT_DUR2, 0x7F); //tap duraction
  myIMU.writeRegister(LSM6DS3_ACC_GYRO_MD1_CFG, 0b01000000);  //single tap route to int
}
