#include <ArduinoBLE.h>
#include <Adafruit_NeoPixel.h>

BLEService ledService("19B10000-E8F2-537E-4F6C-D104768A1214"); // Bluetooth® Low Energy LED Service

// Bluetooth® Low Energy LED Switch Characteristic - custom 128-bit UUID, read and writable by central
BLECharacteristic switch1Characteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLEWrite, 15);
BLECharacteristic switch2Characteristic("19B10001-E8F2-537E-4F6C-D104768A1215", BLEWrite, 15);


Adafruit_NeoPixel pixels2(22, D2, NEO_GRB + NEO_KHZ800);


uint8_t character_data1[15] = {0};
uint8_t character_data2[15] = {0};

void setup() {

  Serial.begin(9600);
  pixels2.begin();
  //while (!Serial);

  // set LED pin to output mode
  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);
  digitalWrite(LEDR, HIGH);
  digitalWrite(LEDG, HIGH);
  digitalWrite(LEDB, HIGH);
  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");
    while (1);
  }

  // set advertised local name and service UUID:
  BLE.setLocalName("AUTOMATON");
  BLE.setAdvertisedService(ledService);

  // add the characteristic to the service
  ledService.addCharacteristic(switch1Characteristic);
  ledService.addCharacteristic(switch2Characteristic);

  // add service
  BLE.addService(ledService);

  // set the initial value for the characeristic:

  switch1Characteristic.writeValue(character_data1, 15 , false);
  switch2Characteristic.writeValue(character_data2, 15 , false);
  // start advertising

  Serial.println("BLE LED Peripheral");
}

bool advertising = false;
uint32_t one_seen = 0;
uint32_t two_seen = 0;
int fps0 = 0;
int fps1 = 0;
int fps2 = 0;

void loop() {
  static int total_time = 0;
  uint32_t start_it = micros();

  BLE.central();

  // listen for Bluetooth® Low Energy peripherals to connect:



  // if a central is connected to peripheral:
  if (switch1Characteristic.written()) {
    one_seen = millis();
    fps1++;
    size_t len = switch1Characteristic.valueLength();
    const unsigned char *data = switch1Characteristic.value();
    memcpy( character_data1, data, len);
  }

  if (switch2Characteristic.written()) {
    fps2++;
    two_seen = millis();
    size_t len = switch2Characteristic.valueLength();
    const unsigned char *data = switch2Characteristic.value();
    memcpy( character_data2, data, len);
  }

  int connected_devices = 0;
  if (millis() - one_seen < 1000) {
    connected_devices++;
  } else {
    digitalWrite(LEDR, HIGH);
  }
  if (millis() - two_seen < 1000) {
    connected_devices++;
  } else {
    digitalWrite(LEDG, HIGH);
  }


  digitalWrite(LEDR, character_data1[0] & 0x01);
  digitalWrite(LEDG, character_data2[0] & 0x01);

  total_time += micros() - start_it;

  pixels2.clear();
  for (int i = 0; i < 8; i++) {
    pixels2.setPixelColor( i, pixels2.Color(character_data2[i + 1],  character_data1[i + 1], 0)  );
    pixels2.setPixelColor(16 - 1 - i, pixels2.Color( character_data2[i + 1],  character_data1[i + 1], 0)  );


  }
  pixels2.show();




  static uint32_t last_time = 0;
  if (millis() - last_time > 1000  ) {
    last_time = millis();
    BLE.advertise();
    Serial.print( total_time / 10000);
    Serial.print("% Load ");

    Serial.print( connected_devices);
    Serial.print(" ");
    Serial.print( fps0);
    Serial.print(" ");
    Serial.print( fps1);
    Serial.print(" ");
    Serial.println(fps2);
    fps0 = 0;
    fps1 = 0;
    total_time = 0;
    fps2 = 0;
  }

  fps0++;


}
