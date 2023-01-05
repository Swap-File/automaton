#include <bluefruit.h>
#include "ble.h"

const uint8_t   LBS_UUID_SERVICE[] = { 0x14, 0x12, 0x8A, 0x76, 0x04, 0xD1, 0x6C, 0x4F, 0x7E, 0x53, 0xF2, 0xE8, 0x00, 0x00, 0xB1, 0x19 };
const uint8_t  LBS_UUID_CHR_LEFT[] = { 0x14, 0x12, 0x8A, 0x76, 0x04, 0xD1, 0x6C, 0x4F, 0x7E, 0x53, 0xF2, 0xE8, 0x01, 0x00, 0xB1, 0x19 };
const uint8_t LBS_UUID_CHR_RIGHT[] = { 0x15, 0x12, 0x8A, 0x76, 0x04, 0xD1, 0x6C, 0x4F, 0x7E, 0x53, 0xF2, 0xE8, 0x01, 0x00, 0xB1, 0x19 };
const uint8_t LBS_UUID_CHR_VIBE[] = { 0x16, 0x12, 0x8A, 0x76, 0x04, 0xD1, 0x6C, 0x4F, 0x7E, 0x53, 0xF2, 0xE8, 0x01, 0x00, 0xB1, 0x19 };

BLEClientService  hand_service(LBS_UUID_SERVICE);
BLEClientCharacteristic  hand_characteristic;
BLEClientCharacteristic  vibe_characteristic(LBS_UUID_CHR_VIBE);


bool ble_connected = false;
bool _hand = false;

void vibe_notify_callback(BLEClientCharacteristic* chr, uint8_t* data, uint16_t len) {
  if ( len == 1) {
    if (_hand) {  //left
      if ((data[0] & 0xF0) == 0x00)
        digitalWrite(D0, LOW);
      else
        digitalWrite(D0, HIGH);
    } else { //right
      if ((data[0] & 0x0F) == 0x00)
        digitalWrite(D0, LOW);
      else
        digitalWrite(D0, HIGH);
    }
  }
}

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
    Serial.println("hand_service discovery problem.");
    Bluefruit.Scanner.resume();
    return;
  }

  if (!hand_characteristic.discover()) {
    Serial.println("hand_characteristic discovery problem.");
    Bluefruit.Scanner.resume();
    return;
  }
  if (!vibe_characteristic.discover()) {
    Serial.println("vibe_characteristic discovery problem.");
    Bluefruit.Scanner.resume();
    return;
  }
  if (!vibe_characteristic.enableNotify()) {
    Serial.println("vibe_characteristic enableNotify problem.");
    Bluefruit.Scanner.resume();
    return;
  }
  ble_connected = true;
}

void cent_disconnect_callback(uint16_t conn_handle, uint8_t reason) {
  Serial.println("[Central] Disconnected");
  digitalWrite(D0, LOW);
  ble_connected = false;
  Bluefruit.Scanner.resume();
}

bool ble_send(struct cpu_struct *hand_data) {

  if (ble_connected) {
    uint8_t payload[15] = { 0 };

    payload[14] = hand_data->tap_event;
    payload[13] = hand_data->tap_event_counter;
    payload[12] = hand_data->step_count;

    payload[11] = hand_data->yaw;
    payload[10] = hand_data->pitch;
    payload[9] = hand_data->roll;

    payload[8] = hand_data->fft[7];
    payload[7] = hand_data->fft[6];
    payload[6] = hand_data->fft[5];
    payload[5] = hand_data->fft[4];
    payload[4] = hand_data->fft[3];
    payload[3] = hand_data->fft[2];
    payload[2] = hand_data->fft[1];
    payload[1] = hand_data->fft[0];
    payload[0] = hand_data->msg_count++;

    hand_characteristic.write(payload, 15);
    return true;
  }
  Bluefruit.Scanner.resume();
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

void ble_init(bool hand) {
	
  // vibe motor
  pinMode(D0, OUTPUT);
  digitalWrite(D0, LOW);
  _hand = hand;

  if (_hand) {
    hand_characteristic = BLEClientCharacteristic(LBS_UUID_CHR_LEFT);
  } else {
    hand_characteristic = BLEClientCharacteristic(LBS_UUID_CHR_RIGHT);
  }

  // Initialize Bluefruit with maximum connections as Peripheral = 0, Central = 1
  Bluefruit.begin(0, 1);
  Bluefruit.autoConnLed(false); // don't let bluefruit have the blue led
  Bluefruit.setTxPower(4);    // keep it loud
  Bluefruit.setName("Bluefruit52");
  hand_service.begin();
  hand_characteristic.begin();
  vibe_characteristic.setNotifyCallback(vibe_notify_callback);
  vibe_characteristic.begin();

  // Start Central Scan
  Bluefruit.setConnLedInterval(250);
  Bluefruit.Scanner.filterUuid(LBS_UUID_SERVICE);
  Bluefruit.Scanner.setRxCallback(scan_callback);
  Bluefruit.Scanner.start(0);

  // Callbacks for Central
  Bluefruit.Central.setConnectCallback(cent_connect_callback);
  Bluefruit.Central.setDisconnectCallback(cent_disconnect_callback);
}
