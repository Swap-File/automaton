#include <bluefruit.h>
#include <Adafruit_NeoPixel.h>
#include "LSM6DS3.h"
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

  bool connected;
};



struct cpu_struct cpu_left = { 0 };
struct cpu_struct cpu_right = { 0 };
struct cpu_struct cpu_head = { 0 };

uint8_t wing_data[60] = { 0 };

FusionAhrs ahrs;
#define IMU_SAMPLE_RATE (0.01f)  //lie about sample rate for nice filtering


// max concurrent connections supported by this example
#define MAX_PRPH_CONNECTION   2
uint8_t connection_count = 0;

const uint8_t LBS_UUID_SERVICE[] = { 0x14, 0x12, 0x8A, 0x76, 0x04, 0xD1, 0x6C, 0x4F, 0x7E, 0x53, 0xF2, 0xE8, 0x00, 0x00, 0xB1, 0x19 };
const uint8_t LBS_UUID_CHR_LEFT[] = { 0x14, 0x12, 0x8A, 0x76, 0x04, 0xD1, 0x6C, 0x4F, 0x7E, 0x53, 0xF2, 0xE8, 0x01, 0x00, 0xB1, 0x19 };
const uint8_t LBS_UUID_CHR_RIGHT[] = { 0x15, 0x12, 0x8A, 0x76, 0x04, 0xD1, 0x6C, 0x4F, 0x7E, 0x53, 0xF2, 0xE8, 0x01, 0x00, 0xB1, 0x19 };

BLEService        lbs(LBS_UUID_SERVICE);
BLECharacteristic lsbLEFT(LBS_UUID_CHR_LEFT);
BLECharacteristic lsbRIGHT(LBS_UUID_CHR_RIGHT);

Adafruit_NeoPixel pixels(60, D2, NEO_GRB + NEO_KHZ800);  //60 safe load


#define int1Pin PIN_LSM6DS3TR_C_INT1

volatile static uint32_t last_tap_time = 0;
volatile uint8_t interruptCount = 0;


LSM6DS3 myIMU(I2C_MODE, 0x6A);

void int1ISR() {
  uint32_t tap_time = millis();
  if (tap_time - last_tap_time < 100) {  // too soon
    return;
  }
  interruptCount++;
  last_tap_time = tap_time;
}

inline void update_imu(void) {

  const FusionVector gyroscope = { myIMU.readFloatGyroX(), myIMU.readFloatGyroY(), myIMU.readFloatGyroZ() };         // replace this with actual gyroscope data in degrees/s
  const FusionVector accelerometer = { myIMU.readFloatAccelX(), myIMU.readFloatAccelY(), myIMU.readFloatAccelZ() };  // replace this with actual accelerometer data in g
  FusionAhrsUpdateNoMagnetometer(&ahrs, gyroscope, accelerometer, IMU_SAMPLE_RATE);
  FusionEuler euler = FusionQuaternionToEuler(FusionAhrsGetQuaternion(&ahrs));

  cpu_head.yaw = (uint8_t)constrain(map(euler.angle.yaw + 180, 0, 360, 0, 256), 0, 255);
  cpu_head.pitch = (uint8_t)constrain(map(euler.angle.pitch + 180, 0, 360, 0, 256), 0, 255);
  cpu_head.roll = (uint8_t)constrain(map(euler.angle.roll + 180, 0, 360, 0, 256), 0, 255);

  myIMU.readRegister(&(cpu_head.step_count), LSM6DS3_ACC_GYRO_STEP_COUNTER_L);

  if (interruptCount > 0 && millis() - last_tap_time > 500) {
    cpu_head.tap_event_counter++;
    cpu_head.tap_event = interruptCount;
    interruptCount = 0;
  }
}


void setup()
{
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);

  Serial.begin(115200);
  Serial1.begin(115200);
  //while ( !Serial ) delay(10);   // for nrf52840 with native usb


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



  Serial.println("Bluefruit52 nRF Blinky Example");
  Serial.println("------------------------------\n");

  // Initialize Bluefruit with max concurrent connections as Peripheral = MAX_PRPH_CONNECTION, Central = 0
  Serial.println("Initialise the Bluefruit nRF52 module");
  Bluefruit.begin(MAX_PRPH_CONNECTION, 0);
  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

  // Setup the LED-Button service using
  Serial.println("Configuring the LED-Button Service");

  // Note: You must call .begin() on the BLEService before calling .begin() on
  // any characteristic(s) within that service definition.. Calling .begin() on
  // a BLECharacteristic will cause it to be added to the last BLEService that
  // was 'begin()'ed!
  lbs.begin();

  // Configure Button characteristic
  // Properties = Read + Notify
  // Permission = Open to read, cannot write
  // Fixed Len  = 1 (button state)
  lsbLEFT.setProperties( CHR_PROPS_WRITE);
  lsbLEFT.setPermission(SECMODE_OPEN, SECMODE_OPEN );
  lsbLEFT.setFixedLen(15);
  lsbLEFT.begin();
  lsbLEFT.setWriteCallback(lsbLEFT_write_callback);

  // Configure the LED characteristic
  // Properties = Read + Write
  // Permission = Open to read, Open to write
  // Fixed Len  = 1 (LED state)
  lsbRIGHT.setProperties( CHR_PROPS_WRITE);
  lsbRIGHT.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  lsbRIGHT.setFixedLen(15);
  lsbRIGHT.begin();
  lsbRIGHT.setWriteCallback(lsbRIGHT_write_callback);

  // Setup the advertising packet(s)
  Serial.println("Setting up the advertising");
  startAdv();

  pixels.begin();
}

void payload_to_struct(struct cpu_struct *cpu, const uint8_t *payload) {

  cpu->tap_event = payload[14];
  cpu->tap_event_counter = payload[13];

  cpu->step_count = payload[12];

  cpu->yaw = payload[11];
  cpu->pitch = payload[10];
  cpu->roll = payload[9];

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

void startAdv(void)
{
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();

  // Include HRM Service UUID
  Bluefruit.Advertising.addService(lbs);

  Bluefruit.setName("AUTOMATON");
  Bluefruit.ScanResponse.addName();
  /* Start Advertising
     - Enable auto advertising if disconnected
     - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
     - Timeout for fast mode is 30 seconds
     - Start(timeout) with timeout = 0 will advertise forever (until connected)

     For recommended advertising interval
     https://developer.apple.com/library/content/qa/qa1931/_index.html
  */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds
}

void lsbLEFT_write_callback(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  if (len == 15) {
    cpu_left.msg_time = millis();
    cpu_left.fps++;
    payload_to_struct(&cpu_left, data);
    digitalWrite(LED_RED, cpu_left.msg_count & 0x01);
  }
}

void lsbRIGHT_write_callback(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  if (len == 15) {
    cpu_right.msg_time = millis();
    cpu_right.fps++;
    payload_to_struct(&cpu_right, data);
    digitalWrite(LED_GREEN, cpu_right.msg_count & 0x01);
  }
}

void loop()
{
  static uint32_t imu_time = 0;  // 20fps to match wrists
  if (millis() - imu_time > 50) {
    imu_time = millis();
    update_imu();  //4 to 9 ms
  }


  static uint32_t led_time = 0;
  if (millis() - led_time > 10) {  // 50fps for blending

    //CALCULATE SCENE
    pixels.clear();
    for (int i = 0; i < 8; i++) {
      pixels.setPixelColor(i, pixels.Color(cpu_left.fft[i], cpu_right.fft[i], 0));
      pixels.setPixelColor(16 - 1 - i, pixels.Color(cpu_left.fft[i], cpu_right.fft[i], 0));
    }

    //OUTPUT SCENE
    pixels.show();                 //3 to 10ms max  DMA, must be BT in background
    Serial1.write(wing_data, 60);  // 7 to 10ms at 115200 baud    2ms , worst 5ms  at 500000 baud
    led_time = millis();
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

void connect_check(void) {
  Serial.print("Connection count: ");
  Serial.println(connection_count);
  if (connection_count < MAX_PRPH_CONNECTION)  {
    Serial.println("Keep advertising");
    Bluefruit.Advertising.start(0);
  }
}

// callback invoked when central connects
void connect_callback(uint16_t conn_handle) {
  Serial.println("Device Connected");
  connection_count++;
  connect_check();
}

/**
   Callback invoked when a connection is dropped
   @param conn_handle connection where this event happens
   @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h
*/
void disconnect_callback(uint16_t conn_handle, uint8_t reason) {
  Serial.println("Device Disconnected, reason = 0x"); Serial.println(reason, HEX);
  connection_count--;
  connect_check();
}
