#define SERIAL_BUFFER_SIZE 256
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Servo2.h>
#include <Metro.h>
#include <FastCRC.h>
#include <cobs.h>
#include <EEPROM.h>
#include "common.h"

uint8_t fin_number = 0;

struct fin_struct fin_data = { 0 };

int onboard_neopixel_power = 11;
int onboard_neopixel = 12;

Adafruit_NeoPixel pixels1(3, 26, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels2(1, onboard_neopixel, NEO_GRB + NEO_KHZ800);

int led_b = 25;
int led_g = 16;
int led_r = 17;

int servoPin = 27;

#define MIN_MICROS 500
#define MAX_MICROS 2500

#define FIN_DATA_LEN (13 * 15)
// global stats
static uint8_t cpu2_crc_errors = 0;
uint32_t cpu2_crc_errors_time = 0;
static uint8_t cpu2_framing_errors = 0;
uint32_t cpu2_framing_errors_time = 0;
static FastCRC8 CRC8;
// serial com data
#define INCOMING_BUFFER_SIZE 256
static uint8_t incoming_incoming_decoded_buffer[INCOMING_BUFFER_SIZE];
static uint8_t incoming_index = 0;
static uint8_t incoming_decoded_buffer[INCOMING_BUFFER_SIZE];
uint32_t incoming_time = 0;

static Metro timer_1hz = Metro(1000);

Servo myservo;

// the setup routine runs once when you press reset:
void setup() {

  EEPROM.begin(16);

  fin_number = EEPROM.read(0);

  delay(fin_number * 150);

  Serial.begin(115200);   //usb debug
  Serial1.begin(460800);  //input bus

  pinMode(led_r, OUTPUT);     //onboard red
  pinMode(led_g, OUTPUT);     //onboard green
  pinMode(led_b, OUTPUT);     //onboard green
  digitalWrite(led_r, HIGH);  //off
  digitalWrite(led_g, HIGH);  //off
  digitalWrite(led_b, HIGH);  //off

  pixels1.begin();
  pixels2.begin();

  pinMode(onboard_neopixel_power, OUTPUT);
  digitalWrite(onboard_neopixel_power, HIGH);

  pinMode(servoPin, OUTPUT);
  digitalWrite(servoPin, HIGH);

  fin_data.servo = 90;
  myservo.attach(servoPin, MIN_MICROS, MAX_MICROS);
  myservo.write(fin_data.servo);
  Serial.println("Boot");
}

int fps_led = 0;
int fps_logic = 0;
static uint32_t framecounter = 0;

void read_data_in(void) {

  while (Serial1.available()) {
    // read in a byte
    incoming_incoming_decoded_buffer[incoming_index] = Serial1.read();
    // check for end of packet
    if (incoming_incoming_decoded_buffer[incoming_index] == 0x00) {
      // try to decode
      uint8_t decoded_length = COBSdecode(incoming_incoming_decoded_buffer, incoming_index, incoming_decoded_buffer);
      // check length of decoded data (cleans up a series of 0x00 bytes)
      if (decoded_length > 0) {
        // check for framing errors
        if (decoded_length != (FIN_DATA_LEN + 1)) {
          cpu2_framing_errors++;
          cpu2_framing_errors = millis();

        } else {
          // check for crc errors
          byte crc = CRC8.maxim(incoming_decoded_buffer, decoded_length - 1);
          if (crc != incoming_decoded_buffer[decoded_length - 1]) {
            cpu2_crc_errors++;
            cpu2_crc_errors_time = millis();

          } else {

            incoming_time = millis();

            int idx = fin_number * sizeof(fin_struct);
            fin_data.strip[0].r = incoming_decoded_buffer[idx++];
            fin_data.strip[0].g = incoming_decoded_buffer[idx++];
            fin_data.strip[0].b = incoming_decoded_buffer[idx++];
            fin_data.strip[1].r = incoming_decoded_buffer[idx++];
            fin_data.strip[1].g = incoming_decoded_buffer[idx++];
            fin_data.strip[1].b = incoming_decoded_buffer[idx++];
            fin_data.strip[2].r = incoming_decoded_buffer[idx++];
            fin_data.strip[2].g = incoming_decoded_buffer[idx++];
            fin_data.strip[2].b = incoming_decoded_buffer[idx++];
            fin_data.led.r = incoming_decoded_buffer[idx++];
            fin_data.led.g = incoming_decoded_buffer[idx++];
            fin_data.led.b = incoming_decoded_buffer[idx++];
            fin_data.servo = incoming_decoded_buffer[idx++];

            framecounter++;
            fps_led++;
          }
        }
      }

      // reset index
      incoming_index = 0;
    } else {
      // read data in until we hit overflow then start over
      incoming_index++;
      if (incoming_index == INCOMING_BUFFER_SIZE) incoming_index = 0;
    }
  }
}

// the loop routine runs over and over again forever:
void loop() {
  static bool serial_connected = false;

  fps_logic++;
  read_data_in();

  if (timer_1hz.check()) {
    Serial.print("Fin ID: ");
    Serial.print(fin_number);
    Serial.print(" ");
    Serial.print(cpu2_framing_errors);
    Serial.print(" ");
    Serial.print(cpu2_crc_errors);
    Serial.print(" ");
    Serial.print(fps_led);
    Serial.print(" ");
    Serial.println(fps_logic);
    fps_led = 0;
    fps_logic = 0;
  }
  if (Serial.available()) {
    Serial.read();
    fin_number++;
    if (fin_number > 14)
      fin_number = 0;
    EEPROM.write(0, fin_number);
    EEPROM.commit();
  }



  if (millis() - incoming_time > 500) {
    if (serial_connected == true) {
      memset(&fin_data, 0, sizeof(fin_data));
      serial_connected = false;
    }
  } else {
    if (serial_connected == false) {
      serial_connected = true;
    }
  }

  static bool servo_enabled = true;

  if (servo_enabled) {
    if (fin_data.servo == 0 || fin_data.servo == 255 || serial_connected == false) {
      myservo.disable();
      servo_enabled = false;
    }
  }

  if (serial_connected && fin_data.servo > 0 && fin_data.servo < 255) {
    myservo.write(fin_data.servo);
    if (!servo_enabled) {
      servo_enabled = true;
      myservo.enable();
    }
  }

  //indicator LEDS
  if (serial_connected)
    digitalWrite(led_r, (framecounter >> 1) & 1);
  else
    digitalWrite(led_r, (millis() >> 8) & 1);

  if (millis() - cpu2_framing_errors_time < 1000)
    digitalWrite(led_b, LOW);
  else
    digitalWrite(led_b, HIGH);

  if (millis() - cpu2_crc_errors_time < 1000)
    digitalWrite(led_g, LOW);
  else
    digitalWrite(led_g, HIGH);

  //handle LEDS
  pixels1.setPixelColor(0, pixels1.Color(fin_data.strip[0].r, fin_data.strip[0].g, fin_data.strip[0].b));
  pixels1.setPixelColor(1, pixels1.Color(fin_data.strip[1].r, fin_data.strip[1].g, fin_data.strip[1].b));
  pixels1.setPixelColor(2, pixels1.Color(fin_data.strip[2].r, fin_data.strip[2].g, fin_data.strip[2].b));
  pixels2.setPixelColor(0, pixels2.Color(fin_data.led.r, fin_data.led.g, fin_data.led.b));

  pixels1.show();
  pixels2.show();
}
