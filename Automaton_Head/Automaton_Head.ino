#define SERIAL_BUFFER_SIZE 256  //big enough for an entire datapoint
#include <bluefruit.h>
#include <Metro.h>
#include "common.h"
#include "imu.h"
#include "fin.h"
#include "ble.h"
#include "gesture.h"
#include "leds.h"
#include "mem.h"

struct led_struct led_data;

Metro metro_1hz = Metro(1000);
Metro metro_20hz = Metro(50);
Metro metro_50hz = Metro(20);
Metro metro_100hz = Metro(10);

struct cpu_struct cpu_left = { 0 };
struct cpu_struct cpu_right = { 0 };
struct cpu_struct cpu_head = { 0 };

void setup() {
  cpu_left.id = 'L';
  cpu_right.id = 'R';
  cpu_head.id = 'H';

  Serial.begin(115200);   //debug
  Serial1.begin(460800);  //wings 115200, 230400, 250000, 460800, 921600, or 1000000
  // while ( !Serial ) delay(10);   // for nrf52840 with native usb

  imu_init();
  ble_init(&cpu_left, &cpu_right);
  leds_init();
  mem_init();
}

void loop() {
  static int notify_fps = 0;
  static int led_fps = 0;
  cpu_head.fps++;

  if (metro_20hz.check()) {
    gesture_check(&cpu_left, &cpu_right, &cpu_head);
    ble_notify(cpu_left.vibe, cpu_right.vibe);
    notify_fps++;
  }

  if (metro_100hz.check()) {
    imu_update(&cpu_head);  //4 to 9 ms
  }


  //fin test
  static uint32_t servo_time = 0;
  static bool fins_up = false;
  if (millis() - servo_time > 5000) {
    servo_time = millis();
    fins_up = !fins_up;
    for (int i; i < FIN_NUM; i++) {
      //if (fins_up) {
      //  led_data.servos[i] = 255;
      //} else {
      //  led_data.servos[i] = 0;
      //}
    }
  }

  if (metro_50hz.check()) {


    //led test
    //for (int i = 0; i < 8; i++) {
    //   led_data.x_leds[i] = CRGB(cpu_left.fft[i], cpu_right.fft[i], 0);
    //   led_data.x_leds[16 - 1 - i] = CRGB(cpu_left.fft[i], cpu_right.fft[i], 0);
    //}

    static uint8_t gHue = 0;
    static int blocker = 0;

    fill_rainbow(led_data.x_leds, X_LED_NUM, gHue, 7);

    EVERY_N_MILLISECONDS(20) {
      gHue++;
    }
    EVERY_N_SECONDS(1) {
      blocker++;
      if (blocker >= X_LED_NUM)
        blocker = 0;
    }
    led_data.x_leds[blocker] = CRGB(0, 0, 0);
    fadeToBlackBy(led_data.x_leds, X_LED_NUM, 192);
    leds_update(&led_data);
    fin_update(&led_data);
    led_fps++;
  }

  EVERY_N_SECONDS(5) {
    static bool servo_pos = true;
    for (int i = 0; i < 15; i++) {
      // if (servo_pos)
      led_data.servos[i] = 0;  //down
      //else
      // led_data.servos[i] = 0;    //up
    }
    servo_pos = !servo_pos;
  }

  //STATS
  if (metro_1hz.check()) {
    Serial.print(cpu_head.fps);
    Serial.print(" ");
    Serial.print(led_fps);
    Serial.print(" ");
    Serial.print(notify_fps);
    Serial.print(" ");
    Serial.print(cpu_left.fps);
    Serial.print(" ");
    Serial.println(cpu_right.fps);
    cpu_head.fps = 0;
    notify_fps = 0;
    led_fps = 0;
    cpu_right.fps = 0;
    cpu_left.fps = 0;
  }

  mem_update();
}
