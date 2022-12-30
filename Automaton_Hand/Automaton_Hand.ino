#include <bluefruit.h>
#include "common.h"
#include "imu.h"
#include "sound.h"
#include "vibe.h"

const uint8_t   LBS_UUID_SERVICE[] = { 0x14, 0x12, 0x8A, 0x76, 0x04, 0xD1, 0x6C, 0x4F, 0x7E, 0x53, 0xF2, 0xE8, 0x00, 0x00, 0xB1, 0x19 };
const uint8_t  LBS_UUID_CHR_LEFT[] = { 0x14, 0x12, 0x8A, 0x76, 0x04, 0xD1, 0x6C, 0x4F, 0x7E, 0x53, 0xF2, 0xE8, 0x01, 0x00, 0xB1, 0x19 };
const uint8_t LBS_UUID_CHR_RIGHT[] = { 0x15, 0x12, 0x8A, 0x76, 0x04, 0xD1, 0x6C, 0x4F, 0x7E, 0x53, 0xF2, 0xE8, 0x01, 0x00, 0xB1, 0x19 };

BLEClientService        hand_service(LBS_UUID_SERVICE);
BLEClientCharacteristic hand_characteristic;

struct cpu_struct hand_data = { 0 };

//add vibration feedback

bool hand = true;  //true = left & red  false = right and green
int hand_led_pin = 0;
float battery_voltage = 4.2;

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
  
  analogReference(AR_INTERNAL_2_4);  //Vref=2.4V
  analogReadResolution(12);          //12bits

  //enable battery measuring
  pinMode(VBAT_ENABLE, OUTPUT);
  digitalWrite(VBAT_ENABLE, LOW);

  //set the pin to read the battery voltage
  pinMode(PIN_VBAT, INPUT);

  //set battery charge speed to 100mA
  pinMode(22 , OUTPUT);
  digitalWrite(22 , LOW);

  Serial.begin(115200);

  vibe_init();
  imu_init();
  sound_init();

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  if (hand) {
    hand_led_pin = LED_RED;
    hand_characteristic = BLEClientCharacteristic(LBS_UUID_CHR_LEFT);
  } else {
    hand_led_pin = LED_GREEN;
    hand_characteristic = BLEClientCharacteristic(LBS_UUID_CHR_RIGHT);
  }

  // Initialize Bluefruit with maximum connections as Peripheral = 0, Central = 1
  Bluefruit.begin(0, 1);
  Bluefruit.autoConnLed(false); // don't let bluefruit have the blue led
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
  //read voltage measure 0.03 v low (experimentally)

  double vBat = ((((float)analogRead(PIN_VBAT)) * 2.4) / 4096.0) * 1510.0 / 510.0; // Voltage divider from Vbat to ADC
  battery_voltage = battery_voltage * 0.95 + 0.05 * vBat;

  if (sound_update(&hand_data)) {  // updates at 20hz set by sample rate and buffer size
    imu_update(&hand_data);      //update the IMU at the same cadence as the microphone
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
  static int loop_time_total = 0;
  uint32_t loop_start_time = micros();

  if (update_mic_and_imu()) {

    if (Bluefruit.Central.connected()) {
      uint8_t payload[15] = { 0 };

      payload[14] = hand_data.tap_event;
      payload[13] = hand_data.tap_event_counter;
      payload[12] = hand_data.step_count;

      payload[11] = hand_data.yaw;
      payload[10] = hand_data.pitch;
      payload[9] = hand_data.roll;

      payload[8] = hand_data.fft[7];
      payload[7] = hand_data.fft[6];
      payload[6] = hand_data.fft[5];
      payload[5] = hand_data.fft[4];
      payload[4] = hand_data.fft[3];
      payload[3] = hand_data.fft[2];
      payload[2] = hand_data.fft[1];
      payload[1] = hand_data.fft[0];
      payload[0] = hand_data.msg_count++;

      hand_characteristic.write(payload, 15);
      digitalWrite(hand_led_pin, hand_data.msg_count & 0x01);

      //battery empty indicator when connected
      if (battery_voltage < 3.6) {
        digitalWrite(LED_BLUE, !(hand_data.msg_count & 0x01));
      } else {
        digitalWrite(LED_BLUE, HIGH);
      }

    } else {

      //battery full indicator when disconnected
      if (battery_voltage > 4.0) {
        digitalWrite(LED_BLUE, !(millis() >> 8 & 0x01));
      } else {
        digitalWrite(LED_BLUE, HIGH);
      }
      digitalWrite(hand_led_pin, millis() >> 8 & 0x01);

    }
    hand_data.fps++;
    loop_time_total += micros() - loop_start_time;
  }
  static int tap_event_counter_last = 0 ;
  if (hand_data.tap_event_counter != tap_event_counter_last){
    tap_event_counter_last = hand_data.tap_event_counter;
    vibe_add_quick_slow(hand_data.tap_event,0);
  }
  vibe_update();
  
  static uint32_t fps_time = 0;
  if (millis() - fps_time > 1000) {
    fps_time += 1000;
    if (millis() - fps_time > 1000)
      fps_time = millis();
    Serial.print("\t");
    Serial.print(battery_voltage);
    Serial.print("\t");
    Serial.print(loop_time_total / 10000);
    Serial.print("% Load \tFPS:");
    Serial.println(hand_data.fps);
    hand_data.fps = 0;
    loop_time_total = 0;
  }
}
