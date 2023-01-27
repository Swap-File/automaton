#ifndef _LEDS_H
#define _LEDS_H
#include <FastLED.h>

#include "common.h"


void leds_update(struct led_struct * led_data);
void leds_init(void);

#endif
