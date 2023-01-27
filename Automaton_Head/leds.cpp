#include <Adafruit_NeoPixel2.h> //modified library with async DMA for NRF52
#include <FastLED.h> //use for the CRGB helpers and datatypes
#include "common.h"

Adafruit_NeoPixel pixels(60, D2, NEO_GRB + NEO_KHZ800);  // 60 safe load

void leds_update(struct led_struct * led_data) {


  
    //CALCULATE SCENE - PLACEHOLDER
    pixels.clear();
    for (int i = 0; i < 8; i++) {
      pixels.setPixelColor(i, pixels.Color(led_data->x_leds[i].r, led_data->x_leds[i].g,led_data->x_leds[i].b));
      pixels.setPixelColor(16 - 1 - i, pixels.Color(led_data->x_leds[i].r, led_data->x_leds[i].g,led_data->x_leds[i].b));
    }

    //OUTPUT SCENE - PLACEHOLDER
    pixels.show();                 //3 to 10ms max  DMA, must be BT in background



}

void leds_init(void){
  pixels.begin();
}
