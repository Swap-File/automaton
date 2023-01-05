#include <bluefruit.h>
#include "vibe.h"
#include "ble.h"

#define EXPIRE_CPU 100

// max concurrent connections supported by this example
#define MAX_PRPH_CONNECTION   2
static uint8_t connection_count = 0;

struct cpu_struct *_cpu_left;
struct cpu_struct *_cpu_right;

const uint8_t LBS_UUID_SERVICE[] = { 0x14, 0x12, 0x8A, 0x76, 0x04, 0xD1, 0x6C, 0x4F, 0x7E, 0x53, 0xF2, 0xE8, 0x00, 0x00, 0xB1, 0x19 };
const uint8_t LBS_UUID_CHR_LEFT[] = { 0x14, 0x12, 0x8A, 0x76, 0x04, 0xD1, 0x6C, 0x4F, 0x7E, 0x53, 0xF2, 0xE8, 0x01, 0x00, 0xB1, 0x19 };
const uint8_t LBS_UUID_CHR_RIGHT[] = { 0x15, 0x12, 0x8A, 0x76, 0x04, 0xD1, 0x6C, 0x4F, 0x7E, 0x53, 0xF2, 0xE8, 0x01, 0x00, 0xB1, 0x19 };
const uint8_t LBS_UUID_CHR_VIBE[] = { 0x16, 0x12, 0x8A, 0x76, 0x04, 0xD1, 0x6C, 0x4F, 0x7E, 0x53, 0xF2, 0xE8, 0x01, 0x00, 0xB1, 0x19 };

BLEService        lbs(LBS_UUID_SERVICE);
BLECharacteristic lsbLEFT(LBS_UUID_CHR_LEFT);
BLECharacteristic lsbRIGHT(LBS_UUID_CHR_RIGHT);
BLECharacteristic lsbVIBE(LBS_UUID_CHR_VIBE);


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

// investigate for safety
void lsbLEFT_write_callback(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  if (len == 15) {
    _cpu_left->msg_time = millis();
    _cpu_left->fps++;
    data[9] = 255 - data[9];  //flip reference
    payload_to_struct(_cpu_left, data);
    digitalWrite(LED_RED,  _cpu_left->msg_count & 0x01);
  }
}

// investigate for safety
void lsbRIGHT_write_callback(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  if (len == 15) {
    _cpu_right->msg_time = millis();
    _cpu_right->fps++;
    data[10] = 255 - data[10];  //flip reference
    payload_to_struct(_cpu_right, data);
    digitalWrite(LED_GREEN, _cpu_right->msg_count & 0x01);
  }
}

void connect_check(void) {
  Serial.print("Connection count: ");
  Serial.println(connection_count);
  if (connection_count < MAX_PRPH_CONNECTION)  {
    Serial.println("Keep advertising");
    Bluefruit.Advertising.start(0);
  }
}

void connect_callback(uint16_t conn_handle) {
  Serial.println(conn_handle);
  Serial.println("Device Connected");
  connection_count++;
  connect_check();
}

void disconnect_callback(uint16_t conn_handle, uint8_t reason) {
  Serial.println("Device Disconnected, reason = 0x"); Serial.println(reason, HEX);
  connection_count--;
  connect_check();
}

void ble_init(struct cpu_struct *cpu_left, struct cpu_struct *cpu_right) {
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);

  _cpu_left = cpu_left;
  _cpu_right = cpu_right;

  // Initialize Bluefruit with max concurrent connections as Peripheral = MAX_PRPH_CONNECTION, Central = 0
  Serial.println("Initialise the Bluefruit nRF52 module");
  Bluefruit.begin(MAX_PRPH_CONNECTION, 0);
  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);
  
  lbs.begin();

  lsbLEFT.setProperties( CHR_PROPS_WRITE_WO_RESP );
  lsbLEFT.setPermission(SECMODE_OPEN, SECMODE_OPEN );
  lsbLEFT.setFixedLen(15);
  lsbLEFT.begin();
  lsbLEFT.setWriteCallback(lsbLEFT_write_callback);

  lsbRIGHT.setProperties( CHR_PROPS_WRITE_WO_RESP  );
  lsbRIGHT.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  lsbRIGHT.setFixedLen(15);
  lsbRIGHT.begin();
  lsbRIGHT.setWriteCallback(lsbRIGHT_write_callback);

  lsbVIBE.setProperties( CHR_PROPS_NOTIFY  );
  lsbVIBE.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  lsbVIBE.setFixedLen(1);
  lsbVIBE.begin();

  Serial.println("Setting up the advertising");

  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();

  // Include HRM Service UUID
  Bluefruit.Advertising.addService(lbs);

  Bluefruit.setName("AUTOMATON");
  Bluefruit.ScanResponse.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds
}

void clear_cpu(struct cpu_struct *cpu) {
  cpu->fft[7] = 0;
  cpu->fft[6] = 0;
  cpu->fft[5] = 0;
  cpu->fft[4] = 0;
  cpu->fft[3] = 0;
  cpu->fft[2] = 0;
  cpu->fft[1] = 0;
  cpu->fft[0] = 0;

}
void expire_cpu(void) {

  if (millis() - _cpu_left->msg_time > EXPIRE_CPU)
    clear_cpu(_cpu_left);

  if (millis() - _cpu_right->msg_time > EXPIRE_CPU)
    clear_cpu(_cpu_right);


}
void ble_notify(bool left, bool right) {
  expire_cpu();

  static bool alternate = false; // interleaved

  // Do we want to handle the requests device side?
  // Is this good enough?

  if (alternate) {
    lsbVIBE.notify8(0, vibe_read(left, right));
  } else {
    lsbVIBE.notify8(1, vibe_read(left, right));
  }
  alternate = !alternate;
}
