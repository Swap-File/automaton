#include <bluefruit.h>
#include "LSM6DS3.h"
#include "Wire.h"
#include "Fusion.h"
#include <arduinoFFT.h>
#include <PDM.h>

const uint8_t LBS_UUID_SERVICE[] = { 0x14, 0x12, 0x8A, 0x76, 0x04, 0xD1, 0x6C, 0x4F, 0x7E, 0x53, 0xF2, 0xE8, 0x00, 0x00, 0xB1, 0x19 };
const uint8_t LBS_UUID_CHR_LEFT[] = { 0x14, 0x12, 0x8A, 0x76, 0x04, 0xD1, 0x6C, 0x4F, 0x7E, 0x53, 0xF2, 0xE8, 0x01, 0x00, 0xB1, 0x19 };
const uint8_t LBS_UUID_CHR_RIGHT[] = { 0x15, 0x12, 0x8A, 0x76, 0x04, 0xD1, 0x6C, 0x4F, 0x7E, 0x53, 0xF2, 0xE8, 0x01, 0x00, 0xB1, 0x19 };

BLEClientService        hand_service(LBS_UUID_SERVICE);
BLEClientCharacteristic hand_characteristic;

//add autohand detection via pins
//add battery level low blue blinking
//add vibration feedback

bool hand = true;  //true = left & red  false = right and green
int hand_led_pin = 0;

float battery_voltage = 4.2;

// sound globals
#define FFT_SAMPLES 16
uint8_t fft_scaled_255[FFT_SAMPLES / 2] = { 0 };
volatile bool pdm_processed = true;

// imu globals
FusionEuler euler;
LSM6DS3 myIMU(I2C_MODE, 0x6A);
FusionAhrs ahrs;
uint8_t stepCount = 0;
int tap_event_count = 0;
int tap_event_last = 0;

void cent_connect_callback(uint16_t conn_handle) {
  BLEConnection* connection = Bluefruit.Connection(conn_handle);
  char peer_name[32] = { 0 };
  connection->getPeerName(peer_name, sizeof(peer_name));
  Serial.print("[Central] Connected to: ");
  Serial.println(peer_name);

  if (strcmp(peer_name, "AUTOMATON") != 0) {
    Serial.println("Name mismatch.");
    Bluefruit.Scanner.resume();
    return;
  }

  if (!hand_service.discover(conn_handle)) {
    Serial.println("Service mismatch.");
    Bluefruit.Scanner.resume();
    return;
  }

  if (!hand_characteristic.discover()) {
    Serial.println("Characteristic mismatch.");
    Bluefruit.Scanner.resume();
    return;
  }

}

void cent_disconnect_callback(uint16_t conn_handle, uint8_t reason) {
  Serial.println("[Central] Disconnected");
  Bluefruit.Scanner.resume();
}

void setup() {

  //enable battery measuring
  pinMode(VBAT_ENABLE, OUTPUT);
  digitalWrite(VBAT_ENABLE, LOW);

  //set the pin to read the battery voltage
  pinMode(PIN_VBAT, INPUT);

  //set battery charge speed to 100mA
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);

  // analogRead(PIN_VBAT);

  Serial.begin(115200);

  imu_init();
  sound_init();

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  //blue led is handled directly by bluefruit

  if (hand) {
    hand_led_pin = LED_RED;
    hand_characteristic = BLEClientCharacteristic(LBS_UUID_CHR_LEFT);
  } else {
    hand_led_pin = LED_GREEN;
    hand_characteristic = BLEClientCharacteristic(LBS_UUID_CHR_RIGHT);
  }

 
  // Initialize Bluefruit with maximum connections as Peripheral = 0, Central = 1
  Bluefruit.begin(0, 1);
  Bluefruit.setTxPower(4);    // keep it loud
  Bluefruit.setName("Bluefruit52");
  hand_service.begin();
  hand_characteristic.begin();

  // Start Central Scan
  Bluefruit.setConnLedInterval(250);
  Bluefruit.Scanner.filterUuid(LBS_UUID_SERVICE);
  Bluefruit.Scanner.setRxCallback(scan_callback);
  Bluefruit.Scanner.start(0);

  // Callbacks for Central
  Bluefruit.Central.setConnectCallback(cent_connect_callback);
  Bluefruit.Central.setDisconnectCallback(cent_disconnect_callback);

  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(hand_led_pin, LOW);
}


bool update_mic_and_imu(void) {
  imu_isr_check();

  double vBat = ((analogRead(PIN_VBAT) * 3.3) / 1024) * 1510.0 / 510.0; // Voltage divider from Vbat to ADC
  battery_voltage = battery_voltage * 0.95 + 0.05 * vBat;

  if (update_mic()) {  // updates at 20hz set by sample rate and buffer size
    update_imu();      //update the IMU at the same cadence as the microphone
    return true;
  }
  return false;
}

void scan_callback(ble_gap_evt_adv_report_t* report) {

  if ( Bluefruit.Scanner.checkReportForUuid(report, LBS_UUID_SERVICE) )  {
    Bluefruit.Central.connect(report);
    Serial.println("Automaton detected");

    uint32_t scan_start = millis();
    while ( Bluefruit.Central.connected() == false && millis() - scan_start < 10000) {
    }
    if (Bluefruit.Central.connected() == false ) {
      Serial.println("Connection Failed");
      Bluefruit.Scanner.resume();
      return;
    }
    Serial.print("Connected in ");
    Serial.print(millis() - scan_start);
    Serial.println("ms");
    return;
  }
}


void loop() {
  static uint8_t payload[15] = { 0 };
  static int fps = 0;
  static int total_time = 0;
  uint32_t start_it = micros();

  if (Bluefruit.Central.connected() && update_mic_and_imu()) {

    payload[14] = tap_event_last;
    payload[13] = tap_event_count;
    payload[12] = stepCount;
    payload[11] = constrain(map(euler.angle.yaw + 180, 0, 360, 0, 256), 0, 255);
    payload[10] = constrain(map(euler.angle.pitch + 180, 0, 360, 0, 256), 0, 255);
    payload[9] = constrain(map(euler.angle.roll + 180, 0, 360, 0, 256), 0, 255);
    payload[8] = fft_scaled_255[7];
    payload[7] = fft_scaled_255[6];
    payload[6] = fft_scaled_255[5];
    payload[5] = fft_scaled_255[4];
    payload[4] = fft_scaled_255[3];
    payload[3] = fft_scaled_255[2];
    payload[2] = fft_scaled_255[1];
    payload[1] = fft_scaled_255[0];
    payload[0]++;

    hand_characteristic.write(payload, 15);

    fps++;
    total_time += micros() - start_it;
  }

  // LED update
  if (Bluefruit.Central.connected()) {
    digitalWrite(hand_led_pin, payload[0] & 0x01);
  } else {
    digitalWrite(hand_led_pin, millis() >> 8 & 0x01);
  }

  static uint32_t last_time = 0;
  if (millis() - last_time > 1000) {
    last_time += 1000;
    if (millis() - last_time > 1000)
      last_time = millis();
    Serial.print("\t");
    Serial.print(battery_voltage);
    Serial.print("\t");
    Serial.print(total_time / 10000);
    Serial.print("% Load \tFPS:");
    Serial.println(fps);
    fps = 0;
    total_time = 0;
  }
}
