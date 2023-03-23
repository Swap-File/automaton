#ifndef _FIN_H
#define _FIN_H
#include <FastLED.h>

#include "common.h"

struct fin_struct {

  CRGB strip[3];
  CRGB led;
  uint8_t servo = 90;
  uint8_t servo_sent = 90;
  uint8_t servo_last = 0;
  uint32_t servo_time = 0;
};

void fin_update( struct led_struct* led_data);

// fin positions
#define FIN_MODE_FIRST 0
#define FIN_DOWN 0
#define FIN_MID 1 
#define FIN_UP 2
#define FIN_MODE_LAST 2
// fin movement style

#define FIN_EFFECT_FIRST 0
#define FIN_IMMEDIATE 0
#define FIN_LEFT 1
#define FIN_RIGHT 2
#define FIN_ALT 3
#define FIN_EFFECT_LAST 3

#endif
