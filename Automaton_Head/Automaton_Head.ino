#define SERIAL_BUFFER_SIZE 256  //big enough for an entire datapoint
#include <bluefruit.h>
#include <Metro.h>
#include "common.h"
#include "imu.h"
#include "fin.h"
#include "ble.h"
#include "leds.h"
#include "mem.h"
#include "effects.h"
#include "logic.h"
#include "sound.h"

#include <FastLED.h>
struct led_struct led_data;
#define SMOOTH_ROLL 0.8
#define SMOOTH_PITCH 0.2

Metro metro_1hz = Metro(1000);
Metro metro_20hz = Metro(50);
Metro metro_50hz = Metro(20);
Metro metro_100hz = Metro(10);

struct cpu_struct cpu_left = { 0 };
struct cpu_struct cpu_right = { 0 };
struct cpu_struct cpu_head = { 0 };

extern uint8_t connection_count;
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
  sound_init();
}

void loop() {
  static int notify_fps = 0;
  static int led_fps = 0;
  cpu_head.fps++;

  logic_update(&led_data, &cpu_left, &cpu_right, &cpu_head);

  if (metro_20hz.check()) {
    ble_notify(cpu_left.vibe, cpu_right.vibe);
    notify_fps++;

    cpu_left.pitch_smoothed = cpu_left.pitch_smoothed * SMOOTH_PITCH + cpu_left.pitch * (1 - SMOOTH_PITCH);
    cpu_left.roll_smoothed = cpu_left.roll_smoothed * SMOOTH_ROLL + cpu_left.roll * (1 - SMOOTH_ROLL);
    cpu_right.pitch_smoothed = cpu_right.pitch_smoothed * SMOOTH_PITCH + cpu_right.pitch * (1 - SMOOTH_PITCH);
    cpu_right.roll_smoothed = cpu_right.roll_smoothed * SMOOTH_ROLL + cpu_right.roll * (1 - SMOOTH_ROLL);


    led_data.left_roll = map(cpu_left.roll_smoothed, 64, 192, 8 * 127, -8 * 127);    //vibe at edges and middle?  add deadzone
    led_data.right_roll = map(cpu_right.roll_smoothed, 64, 192, -8 * 127, 8 * 127);  //vibe at edges and middle?  add deadzone


    led_data.left_pitch = ((cpu_left.pitch_smoothed - cpu_left.pitch) * 10);
    led_data.right_pitch = ((cpu_right.pitch_smoothed - cpu_right.pitch) * 10);

    led_data.left_pitch = 4* ( 127 -( cpu_left.pitch) );  
    led_data.right_pitch = 4* ( 127- (cpu_right.pitch) );


    Serial.print(led_data.left_pitch);
    Serial.print('\t');
    Serial.print(led_data.right_pitch);
    Serial.print('\t');
    Serial.println('\t');
  }


  if (metro_100hz.check()) {
    imu_update(&cpu_head);  //4 to 9 ms
  }

  sound_update(&cpu_head);

  if (metro_50hz.check()) {
    effects_update(&led_data, &cpu_left, &cpu_right, &cpu_head);
    leds_update(&led_data);
    fin_update(&led_data);
    led_fps++;
  }

  //STATS
  if (metro_1hz.check()) {  // normal is: 12800-130000,  20 , 20 , 20
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
