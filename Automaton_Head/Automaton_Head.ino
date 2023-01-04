#include <bluefruit.h>
#include <Adafruit_NeoPixel.h>
#include <Metro.h>

#include "common.h"
#include "imu.h"
#include "vibe.h"
#include "ble.h"
#include "gesture.h"

Metro metro_1hz = Metro(1000);
Metro metro_20hz = Metro(50);
Metro metro_100hz = Metro(10);

uint8_t wing_data[60] = { 0 };

struct cpu_struct cpu_left = { 0 };
struct cpu_struct cpu_right = { 0 };
struct cpu_struct cpu_head = { 0 };

Adafruit_NeoPixel pixels(60, D2, NEO_GRB + NEO_KHZ800);  // 60 safe load

void setup()
{
  Serial.begin(115200);
  Serial1.begin(115200);
  // while ( !Serial ) delay(10);   // for nrf52840 with native usb

  imu_init();
  ble_init(&cpu_left, &cpu_right);

  pixels.begin();
}



void loop()
{
  static int notify_fps = 0;
  static int led_fps = 0;
  cpu_head.fps++;

  if (metro_20hz.check()) {
    vibe_update(&cpu_left.vibe, &cpu_right.vibe);
    ble_notify(cpu_left.vibe, cpu_right.vibe);
    notify_fps++;
    gesture_check(&cpu_left, &cpu_right);

  }

  if (metro_100hz.check()) {
    imu_update(&cpu_head);  //4 to 9 ms

    //CALCULATE SCENE - PLACEHOLDER
    pixels.clear();
    for (int i = 0; i < 8; i++) {
      pixels.setPixelColor(i, pixels.Color(cpu_left.fft[i], cpu_right.fft[i], 0));
      pixels.setPixelColor(16 - 1 - i, pixels.Color(cpu_left.fft[i], cpu_right.fft[i], 0));
    }

    //OUTPUT SCENE - PLACEHOLDER
    pixels.show();                 //3 to 10ms max  DMA, must be BT in background
    Serial1.write(wing_data, 60);  // 7 to 10ms at 115200 baud    2ms , worst 5ms  at 500000 baud
    led_fps++;
  }

  //VIBE - PLACEHOLDER
  static int left_tap_event_counter = 0;
  static int right_tap_event_counter = 0;

  if (left_tap_event_counter != cpu_left.tap_event_counter) {
    vibe_add_quick_slow(VIBE_LEFT, 1, 0);
    left_tap_event_counter =  cpu_left.tap_event_counter;
  }

  if (right_tap_event_counter != cpu_right.tap_event_counter) {
    vibe_add_quick_slow(VIBE_RIGHT, 1, 0);
    right_tap_event_counter = cpu_right.tap_event_counter;
  }

  //STATS
  if (metro_1hz.check()) {
    /*
      Serial.print(cpu_head.fps);
      Serial.print(" ");
      Serial.print(led_fps);
      Serial.print(" ");
      Serial.print(notify_fps);
      Serial.print(" ");
      Serial.print(cpu_left.fps);
      Serial.print(" ");
      Serial.println(cpu_right.fps);
    */
    cpu_head.fps = 0;
    notify_fps = 0;
    led_fps = 0;
    cpu_right.fps = 0;
    cpu_left.fps = 0;
  }

}
