#ifndef _WING_H
#define _WING_H
#include <FastLED.h>

#include "common.h"


//allow per servo settings
const uint8_t servo_max[15] = {  30, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45};
const uint8_t servo_min[15] = { 120,135,135,135,135,135,135,135,135,135,135,135,135,135,135};


struct fin_struct {

  CRGB strip[3];
  CRGB led;
  uint8_t servo = 90;

};

void fin_update(struct led_struct * led_data);

#endif
