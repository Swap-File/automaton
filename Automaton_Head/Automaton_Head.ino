#include <bluefruit.h>
#include <Adafruit_NeoPixel.h>
#include <Metro.h>

#include "common.h"
#include "imu.h"

Metro notify_timer = Metro(50);

struct cpu_struct cpu_left = { 0 };
struct cpu_struct cpu_right = { 0 };
struct cpu_struct cpu_head = { 0 };

uint8_t wing_data[60] = { 0 };

// max concurrent connections supported by this example
#define MAX_PRPH_CONNECTION   2
uint8_t connection_count = 0;

const uint8_t LBS_UUID_SERVICE[] = { 0x14, 0x12, 0x8A, 0x76, 0x04, 0xD1, 0x6C, 0x4F, 0x7E, 0x53, 0xF2, 0xE8, 0x00, 0x00, 0xB1, 0x19 };
const uint8_t LBS_UUID_CHR_LEFT[] = { 0x14, 0x12, 0x8A, 0x76, 0x04, 0xD1, 0x6C, 0x4F, 0x7E, 0x53, 0xF2, 0xE8, 0x01, 0x00, 0xB1, 0x19 };
const uint8_t LBS_UUID_CHR_RIGHT[] = { 0x15, 0x12, 0x8A, 0x76, 0x04, 0xD1, 0x6C, 0x4F, 0x7E, 0x53, 0xF2, 0xE8, 0x01, 0x00, 0xB1, 0x19 };
const uint8_t LBS_UUID_CHR_VIBE[] = { 0x16, 0x12, 0x8A, 0x76, 0x04, 0xD1, 0x6C, 0x4F, 0x7E, 0x53, 0xF2, 0xE8, 0x01, 0x00, 0xB1, 0x19 };

BLEService        lbs(LBS_UUID_SERVICE);
BLECharacteristic lsbLEFT(LBS_UUID_CHR_LEFT);
BLECharacteristic lsbRIGHT(LBS_UUID_CHR_RIGHT);
BLECharacteristic lsbVIBE(LBS_UUID_CHR_VIBE);

Adafruit_NeoPixel pixels(60, D2, NEO_GRB + NEO_KHZ800);  //60 safe load

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
void setup()
{
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);

  Serial.begin(115200);
  Serial1.begin(115200);
  // while ( !Serial ) delay(10);   // for nrf52840 with native usb

  imu_init();

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
  startAdv();

  pixels.begin();
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
    imu_update(&cpu_head);  //4 to 9 ms
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

  static uint8_t vibe = 0x0F;
  //STATS
  static uint32_t last_time = 0;
  if (millis() - last_time > 1000) {
    last_time = millis();


    Serial.print(cpu_head.fps);
    Serial.print(" ");
    Serial.print(cpu_left.fps);
    Serial.print(" ");
    Serial.println(cpu_right.fps);

    static int left_tap_event_counter = 0;
    static int right_tap_event_counter = 0;

    if (cpu_left.vibe)
      cpu_left.vibe = false;

    if (cpu_right.vibe)
      cpu_right.vibe = false;

    if (left_tap_event_counter != cpu_left.tap_event_counter) {
      cpu_left.vibe = true;
      left_tap_event_counter = cpu_left.tap_event_counter;
    }

    if (right_tap_event_counter != cpu_right.tap_event_counter) {
      cpu_right.vibe = true;
      right_tap_event_counter = cpu_right.tap_event_counter;
    }

    cpu_left.fps = 0;
    cpu_right.fps = 0;
    cpu_head.fps = 0;
  }

  cpu_head.fps++;

  if (notify_timer.check()) {  // 100hz

    static bool alternate = false; // interleaved for 50hz each
    uint8_t data = 0x00;

    if (cpu_left.vibe && cpu_right.vibe)
      data = 0xFF;
    else if (cpu_right.vibe)
      data = 0x0F;
    else if (cpu_left.vibe)
      data = 0xF0;
      
    if (alternate) {
      lsbVIBE.notify8(0, data);
    } else {
      lsbVIBE.notify8(1, data);
    }
    alternate = !alternate;
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
