#ifndef _WING_H
#define _WING_H
#include <FastLED.h>

#include "common.h"



struct fin_struct {

  CRGB strip[3];
  CRGB led;
  uint8_t servo = 90;
};

void fin_update(struct led_struct* led_data);

#endif
