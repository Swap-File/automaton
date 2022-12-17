#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <RP2040_ISR_Servo.h>
#include <Metro.h>
#include <FastCRC.h>
#include <cobs.h>

int onboard_neopixel_power = 11;
int onboard_neopixel  = 12;

Adafruit_NeoPixel pixels1(1, onboard_neopixel, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels2(2, 29, NEO_GRB + NEO_KHZ800);


int led_b = 25;
int led_g = 16;
int led_r = 17;

int servoIndex = -1;
int servoPin = 27;


#define MIN_MICROS        500
#define MAX_MICROS        2500

// global stats
static uint8_t cpu2_crc_errors = 0;
static uint8_t cpu2_framing_errors = 0;
static FastCRC8 CRC8;
// serial com data

#define INCOMING_BUFFER_SIZE 256
static uint8_t incoming_raw_buffer[INCOMING_BUFFER_SIZE];
static uint8_t incoming_index = 0;
static uint8_t incoming_decoded_buffer[INCOMING_BUFFER_SIZE];

static Metro timer_1hz = Metro(1000);
static Metro timer_100hz = Metro(10);


// the setup routine runs once when you press reset:
void setup() {
  delay(1000);
  
  Serial.begin(115200); //usb debug
  Serial1.begin(500000); //input bus
  
  pinMode(led_r, OUTPUT); //onboard red
  pinMode(led_g, OUTPUT); //onboard green
  pinMode(led_b, OUTPUT); //onboard green
  digitalWrite(led_r, HIGH); //off
  digitalWrite(led_g, HIGH); //off
  digitalWrite(led_b, HIGH); //off

  pixels1.begin();
  pixels2.begin();

  pinMode(onboard_neopixel_power, OUTPUT);
  digitalWrite(onboard_neopixel_power, HIGH);

  pinMode(servoPin, OUTPUT);
  digitalWrite(servoPin, LOW);
  servoIndex = RP2040_ISR_Servos.setupServo(servoPin, MIN_MICROS, MAX_MICROS);
  
  Serial.println("Boot");
}

int fps_led = 0;
int fps_logic = 0;

void read_data_in(void) {
  
  while (Serial1.available()) {
    // read in a byte
    incoming_raw_buffer[incoming_index] = Serial1.read();
    Serial.write(incoming_raw_buffer[incoming_index]);
    // check for end of packet
    if (incoming_raw_buffer[incoming_index] == 0x00) {
      // try to decode
      uint8_t decoded_length = COBSdecode(incoming_raw_buffer, incoming_index, incoming_decoded_buffer);

      // check length of decoded data (cleans up a series of 0x00 bytes)
      if (decoded_length > 0) {
        // check for framing errors
        if (decoded_length != 9) {
          cpu2_framing_errors++;
        } else {
          // check for crc errors
          byte crc = CRC8.maxim(incoming_decoded_buffer, decoded_length - 1);
          if (crc != incoming_decoded_buffer[decoded_length - 1]) {
            cpu2_crc_errors++;
          } else {

            Serial.print("Got Data");
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
  fps_logic++;
  read_data_in();


  static uint8_t led_colors = 0;
  if (timer_100hz.check()) {
    led_colors++;
  }
  
  if (timer_1hz.check()) {
    Serial.print(fps_led);
    Serial.print(" ");
    Serial.println(fps_logic);
    fps_led = 0;
    fps_logic = 0;
  }
  RP2040_ISR_Servos.setPosition(servoIndex, 0 );

  pixels1.fill(pixels1.Color(0, 127, 0));
  pixels2.fill(pixels2.Color(0, 127, 0));


  pixels1.show();
  pixels2.show();
}
