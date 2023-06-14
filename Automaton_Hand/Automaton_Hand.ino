#include <Metro.h>
#include "common.h"
#include "imu.h"
#include "sound.h"
#include "ble.h"

Metro fps_timer = Metro(1000);
Metro imu_timer = Metro(10);

struct cpu_struct hand_data = { 0 };

const bool hand = true;  // true = left & red  false = right and green

int hand_led_pin = 0;
float battery_voltage = 3.8;

void setup() {
  //battery meter adc settings
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

  //debug
  Serial.begin(115200);

  imu_init();
  sound_init();

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  hand_led_pin = ( hand ) ? LED_RED : LED_GREEN;

  ble_init(hand);

  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(hand_led_pin, LOW);
}


void loop() {
  static int main_fps = 0;
  main_fps++;
  static int imu_fps = 0;

  if (imu_timer.check()) {
    imu_update(&hand_data);
    imu_fps++;
  }


  if (sound_update(&hand_data)) { // updates at 20hz set by sample rate and buffer

    if (ble_send(&hand_data)) {

      digitalWrite(hand_led_pin, hand_data.msg_count & 0x01);

      //battery empty indicator when connected
      if (battery_voltage < 3.6) {
        digitalWrite(LED_BLUE, !(hand_data.msg_count & 0x01));
      } else {
        digitalWrite(LED_BLUE, HIGH);
      }

    } else {
      hand_data.vibe = false;

      //battery full indicator when disconnected
      if (battery_voltage > 4.0) {
        digitalWrite(LED_BLUE, !(millis() >> 8 & 0x01));
      } else {
        digitalWrite(LED_BLUE, HIGH);
      }
      digitalWrite(hand_led_pin, millis() >> 8 & 0x01);

    }
    hand_data.fps++;

    //read voltage measure 0.03 v low (experimentally)
    double vBat;

    vBat = ((((float)analogRead(PIN_VBAT)) * 2.4) / 4096.0) * 1510.0 / 510.0  * 1.027; // Voltage divider from Vbat to ADC

    battery_voltage = battery_voltage * 0.95 + 0.05 * vBat;

  }

  if (fps_timer.check()) {
    Serial.print(imu_fps);
    Serial.print("\t");
    Serial.print(main_fps);
    Serial.print("\t");
    Serial.print(battery_voltage);
    Serial.print("\t");
    Serial.println(hand_data.fps);
    imu_fps = 0;
    hand_data.fps = 0;
    main_fps = 0;
  }
}
