#include <Adafruit_NeoPixel2.h>  //modified library with async DMA for NRF52
#include <FastLED.h>             //use for the CRGB helpers and datatypes
#include "common.h"

#define GEM_LED_LENGTH 51

Adafruit_NeoPixel pixels(GEM_LED_LENGTH, D2, NEO_GRB + NEO_KHZ800);  // 60 safe load test

static CRGB gem_leds[GEM_LED_LENGTH];

void gem_map_x(const CRGB* x_leds) {

  int i = 0;  //back middle bottom
  int j = 21;
  gem_leds[i++] += x_leds[j++];
  gem_leds[i++] += x_leds[j++];
  gem_leds[i++] += x_leds[j++];

  // i = 3;  //back right
  j = 39;
  gem_leds[i++] += x_leds[j++];
  gem_leds[i++] += x_leds[j++];
  gem_leds[i++] += x_leds[j++];

  //  i = 6;  //back left
  j = 5;
  gem_leds[i++] += x_leds[j--];
  gem_leds[i++] += x_leds[j--];
  gem_leds[i++] += x_leds[j--];
  //  i = 9;  //back top
  j = 23;
  gem_leds[i++] += x_leds[j--];
  gem_leds[i++] += x_leds[j--];
  gem_leds[i++] += x_leds[j--];

  //  i = 12;  //front middle
  j = 23;
  gem_leds[i++] += x_leds[j--];
  gem_leds[i++] += x_leds[j--];
  gem_leds[i++] += x_leds[j--];

  //  i = 15;  //front right 1
  j = 20;
  gem_leds[i++] += x_leds[j--];
  gem_leds[i++] += x_leds[j--];
  gem_leds[i++] += x_leds[j--];

  //  i = 18; //front right 2
  j = 14;
  gem_leds[i++] += x_leds[j--];
  gem_leds[i++] += x_leds[j--];
  gem_leds[i++] += x_leds[j--];

  //  i = 21; //front right 3
  j = 8;
  gem_leds[i++] += x_leds[j--];
  gem_leds[i++] += x_leds[j--];
  gem_leds[i++] += x_leds[j--];

  //  i = 22; //front right single
  j = 7;
  gem_leds[i++] += x_leds[j];

  //  i = 23; //front eyebrow
  for (j = 0; j < 8; j++)
    gem_leds[i++] += x_leds[j];


  // i = 31;  //back right
  j = 24;
  gem_leds[i++] += x_leds[j++];
  gem_leds[i++] += x_leds[j++];
  gem_leds[i++] += x_leds[j++];

  // i = 34;  //back right
  j = 30;
  gem_leds[i++] += x_leds[j++];
  gem_leds[i++] += x_leds[j++];
  gem_leds[i++] += x_leds[j++];

  // i = 34;  //back right
  j = 36;
  gem_leds[i++] += x_leds[j++];
  gem_leds[i++] += x_leds[j++];
  gem_leds[i++] += x_leds[j++];

  // i = 34;  //back right
  j = 37;
  gem_leds[i++] += x_leds[j];


  //  i = 23; //front eyebrow
  for (j = 44; j > 36; j--)
    gem_leds[i++] += x_leds[j];
}


void gem_map_y(const CRGB* y_leds) {

  int i = 0;  //back middle bottom
  int j = 21;
  gem_leds[i++] += y_leds[9];
  gem_leds[i++] += y_leds[9];
  gem_leds[i++] += y_leds[9];

  // i = 3;  //back right
  j = 39;
  gem_leds[i++] += y_leds[8];
  gem_leds[i++] += y_leds[8];
  gem_leds[i++] += y_leds[8];

  //  i = 6;  //back left
  j = 5;
  gem_leds[i++] += y_leds[8];
  gem_leds[i++] += y_leds[8];
  gem_leds[i++] += y_leds[8];
  //  i = 9;  //back top
  j = 23;
  gem_leds[i++] += y_leds[7];
  gem_leds[i++] += y_leds[7];
  gem_leds[i++] += y_leds[7];

  //  i = 12;  //front middle
  j = 23;
  gem_leds[i++] += y_leds[2];
  gem_leds[i++] += y_leds[2];
  gem_leds[i++] += y_leds[2];

  //  i = 15;  //front right 1
  j = 20;
  gem_leds[i++] += y_leds[1];
  gem_leds[i++] += y_leds[1];
  gem_leds[i++] += y_leds[1];

  //  i = 18; //front right 2
  j = 14;
  gem_leds[i++] += y_leds[2];
  gem_leds[i++] += y_leds[2];
  gem_leds[i++] += y_leds[2];

  //  i = 21; //front right 3
  j = 8;
  gem_leds[i++] += y_leds[2];
  gem_leds[i++] += y_leds[2];
  gem_leds[i++] += y_leds[2];

  //  i = 22; //front right single
  j = 7;
  gem_leds[i++] +=y_leds[1];

  //  i = 23; //front eyebrow
  for (j = 0; j < 8; j++)
    gem_leds[i++] += y_leds[0];


  // i = 31;  //back right
  j = 24;
  gem_leds[i++] += y_leds[1];
  gem_leds[i++] += y_leds[1];
  gem_leds[i++] += y_leds[1];

  // i = 34;  //back right
  j = 30;
  gem_leds[i++] += y_leds[2];
  gem_leds[i++] += y_leds[2];
  gem_leds[i++] += y_leds[2];

  // i = 34;  //back right
  j = 36;
  gem_leds[i++] += y_leds[2];
  gem_leds[i++] += y_leds[2];
  gem_leds[i++] += y_leds[2];

  // i = 34;  //back right
  j = 37;
  gem_leds[i++] +=y_leds[1];


  //  i = 23; //front eyebrow
  for (j = 44; j > 36; j--)
    gem_leds[i++] += y_leds[0];
}
void gem_render_to_adafruit(void) {
  pixels.clear();
  for (int i = 0; i < GEM_LED_LENGTH; i++) {
    pixels.setPixelColor(i, pixels.Color(gem_leds[i].r, gem_leds[i].g, gem_leds[i].b));
  }
  pixels.show();  //3 to 10ms max  DMA, must be BT in background
}

void leds_update(struct led_struct* led_data) {

  fill_solid(gem_leds, GEM_LED_LENGTH, CRGB(0, 0, 0));
  gem_map_x(led_data->x_leds);
 gem_map_y(led_data->y_leds);
  gem_render_to_adafruit();
}

void leds_init(void) {
  pixels.begin();
}
