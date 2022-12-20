
#include <ArduinoBLE.h>
#include <Adafruit_NeoPixel.h>
#include "LSM6DS3.h"
#include "Wire.h"
#include "Fusion.h"

struct cpu_struct {
  uint8_t fft[8];

  uint8_t yaw;
  uint8_t pitch;
  uint8_t roll;

  uint8_t step_count;

  uint8_t tap_event;
  uint8_t tap_event_counter;

  uint32_t msg_time;
  uint8_t msg_count;

  int fps;
};

struct cpu_struct cpu_left = { 0 };
struct cpu_struct cpu_right = { 0 };
struct cpu_struct cpu_head = { 0 };

FusionAhrs ahrs;
#define IMU_SAMPLE_RATE (0.01f)  //lie about sample rate for nice filtering

BLEService AutomatonService("19B10000-E8F2-537E-4F6C-D104768A1214");  // Bluetooth® Low Energy LED Service

// Bluetooth® Low Energy LED Switch Characteristic - custom 128-bit UUID, read and writable by central
BLECharacteristic LeftHandCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLEWrite, 15);
BLECharacteristic RightHandCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1215", BLEWrite, 15);

#define int1Pin PIN_LSM6DS3TR_C_INT1

int tap_event_count = 0;
int tap_event_last = 0;
volatile static uint32_t last_tap_time = 0;
volatile uint8_t interruptCount = 0;

Adafruit_NeoPixel pixels(60, D2, NEO_GRB + NEO_KHZ800);  //60 safe load

// IMU
FusionEuler euler;
uint8_t stepCount = 0;
LSM6DS3 myIMU(I2C_MODE, 0x6A);

void int1ISR() {
  uint32_t tap_time = millis();
  if (tap_time - last_tap_time < 100) {  // too soon
    return;
  }
  interruptCount++;
  last_tap_time = tap_time;
}


uint8_t wing_data[60] = { 0 };

inline void update_imu(void) {

  const FusionVector gyroscope = { myIMU.readFloatGyroX(), myIMU.readFloatGyroY(), myIMU.readFloatGyroZ() };         // replace this with actual gyroscope data in degrees/s
  const FusionVector accelerometer = { myIMU.readFloatAccelX(), myIMU.readFloatAccelY(), myIMU.readFloatAccelZ() };  // replace this with actual accelerometer data in g
  FusionAhrsUpdateNoMagnetometer(&ahrs, gyroscope, accelerometer, IMU_SAMPLE_RATE);
  euler = FusionQuaternionToEuler(FusionAhrsGetQuaternion(&ahrs));

  myIMU.readRegister(&stepCount, LSM6DS3_ACC_GYRO_STEP_COUNTER_L);
}


void setup() {

  Serial.begin(9600);
  Serial1.begin(500000);
  pixels.begin();

  // set LED pin to output mode
  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);
  digitalWrite(LEDR, HIGH);
  digitalWrite(LEDG, HIGH);
  digitalWrite(LEDB, HIGH);

  FusionAhrsInitialise(&ahrs);

  // Set AHRS algorithm settings
  const FusionAhrsSettings settings = {
    .gain = 0.5f,
    .accelerationRejection = 10.0f,
    .magneticRejection = 0.0f,
    .rejectionTimeout = 5.0f,
  };
  FusionAhrsSetSettings(&ahrs, &settings);


  if (myIMU.begin() != 0) {
    Serial.println("myIMU error");
  } else {
    Serial.println("myIMU OK!");
  }
 //BLE.debug(Serial);
  pinMode(int1Pin, INPUT);
  attachInterrupt(digitalPinToInterrupt(int1Pin), int1ISR, RISING);

  myIMU.writeRegister(LSM6DS3_ACC_GYRO_TAP_CFG1, 0x8E);    // INTERRUPTS_ENABLE, SLOPE_FDS
  myIMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL10_C, 0x3C);    //enable pedometer
  myIMU.writeRegister(LSM6DS3_ACC_GYRO_TAP_THS_6D, 0x8C);  //tap threshold
  //myIMU.writeRegister(LSM6DS3_ACC_GYRO_INT_DUR2, 0x7F); //tap duraction
  myIMU.writeRegister(LSM6DS3_ACC_GYRO_MD1_CFG, 0b01000000);  //single tap route to int


  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");
    while (1)
      ;
  }

  // set advertised local name and service UUID:
  BLE.setLocalName("AUTOMATON");
  BLE.setAdvertisedService(AutomatonService);

  // add the characteristic to the service
  AutomatonService.addCharacteristic(LeftHandCharacteristic);
  AutomatonService.addCharacteristic(RightHandCharacteristic);

  // add service
  BLE.addService(AutomatonService);

}


void payload_to_struct(struct cpu_struct *cpu, const uint8_t *payload) {
  cpu->fft[7] = payload[8];
  cpu->fft[6] = payload[7];
  cpu->fft[5] = payload[6];
  cpu->fft[4] = payload[5];
  cpu->fft[3] = payload[4];
  cpu->fft[2] = payload[3];
  cpu->fft[1] = payload[2];
  cpu->fft[0] = payload[1];
  cpu->msg_count = payload[0];
}


void loop() {
  //GATHER INPUT
  BLE.poll();

  if (LeftHandCharacteristic.written()) {
    size_t len = LeftHandCharacteristic.valueLength();
    if (len == 15) {
      cpu_left.msg_time = millis();
      cpu_left.fps++;
      payload_to_struct(&cpu_left, LeftHandCharacteristic.value());
      digitalWrite(LEDR, cpu_left.msg_count & 0x01);
    }
  }

  BLE.poll();

  if (RightHandCharacteristic.written()) {
    size_t len = RightHandCharacteristic.valueLength();
    if (len == 15) {
      cpu_right.msg_time = millis();
      cpu_right.fps++;
      payload_to_struct(&cpu_right, RightHandCharacteristic.value());
      digitalWrite(LEDG, cpu_right.msg_count & 0x01);
      ;
    }
  }

  int connected_devices = 0;
  if (millis() - cpu_left.msg_time < 10000) {
    connected_devices++;
  } else {
    digitalWrite(LEDR, HIGH);
  }
  if (millis() - cpu_right.msg_time < 10000) {
    connected_devices++;
  } else {
    digitalWrite(LEDG, HIGH);
  }


  static uint32_t imu_time = 0;  // 20fps to match wrists
  if (millis() - imu_time > 50) {
    imu_time = millis();
    BLE.poll();
    update_imu();  //4 to 9 ms
    BLE.poll();
  }


  static uint32_t led_time = 0;
  if (millis() - led_time > 10) {  // 50fps for blending

    //CALCULATE SCENE
    pixels.clear();
    for (int i = 0; i < 8; i++) {
      pixels.setPixelColor(i, pixels.Color(cpu_left.fft[i + 1], cpu_right.fft[i + 1], 0));
      pixels.setPixelColor(16 - 1 - i, pixels.Color(cpu_left.fft[i + 1], cpu_right.fft[i + 1], 0));
    }

    //OUTPUT SCENE
    BLE.poll();
    pixels.show();  //3 to 10ms max  DMA, must be BT in background
    BLE.poll();
    Serial1.write(wing_data, 60);  // 7 to 10ms at 115200 baud    2ms , worst 5ms  at 500000 baud
    BLE.poll();
    led_time = millis();
  }

  static uint32_t adv_time = 0;
  if (millis() - adv_time > 5000 && connected_devices < 2) {
    Serial.println("Advert");
    BLE.poll();
    BLE.advertise();
    BLE.poll();
    adv_time = millis();
  }


  //STATS
  static uint32_t last_time = 0;
  if (millis() - last_time > 1000) {
    last_time = millis();

    Serial.print(cpu_head.fps);
    Serial.print(" ");
    Serial.print(cpu_left.fps);
    Serial.print(" ");
    Serial.println(cpu_right.fps);



    cpu_left.fps = 0;
    cpu_right.fps = 0;
    cpu_head.fps = 0;
  }

  cpu_head.fps++;
}
