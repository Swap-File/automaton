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
#define FIN_UP 2
#define FIN_DOWN 0
#define FIN_MID 1 

// fin movement style
#define FIN_IMMEDIATE 0
#define FIN_LEFT 1
#define FIN_RIGHT 2
#define FIN_ALT 3

#endif
