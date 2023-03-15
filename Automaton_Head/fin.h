#ifndef _WING_H
#define _WING_H
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

void fin_update(struct led_struct* led_data);

#endif
