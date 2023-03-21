#include <Arduino.h>
#include <FastLED.h>
#include "common.h"
#include "effects.h"


CRGBPalette16 LavaColors2_p = {
  CRGB::Orange,
  CRGB::Red,
  CRGB::DarkRed,
  CRGB::DarkRed,
  CRGB::DarkRed,
  CRGB::Red,
  CRGB::Orange,

  CRGB::White,
  CRGB::Orange,
  CRGB::Red,
  CRGB::DarkRed,

  CRGB::DarkRed,
  CRGB::DarkRed,
  CRGB::Red,
  CRGB::Orange,

  CRGB::White,
};

CRGBPalette16 current_palette = PartyColors_p;

int effect_order[] = { 0, 3, 1, 3 };

CRGBPalette16 palette_order[] = { RainbowColors_p, OceanColors_p, PartyColors_p, ForestColors_p, RainbowColors_p, LavaColors2_p };

static int sound_mode = 1;

CRGBPalette16 palette = RainbowColors_p;

void effects_update(struct led_struct *led_data, const struct cpu_struct *cpu_left, const struct cpu_struct *cpu_right) {

  // led_data->sound_brows = true;
  // led_data->sound_gems = false;
  // led_data->sound_fins = true;



  led_data->off = false;

  EVERY_N_SECONDS(18) {  // lazy, should consider start time

    if (sound_mode == 1) {
      sound_mode = 2;
    } else if (sound_mode == 2) {
      sound_mode = 3;
    } else if (sound_mode == 3) {
      sound_mode = 1;
    }
  }

  if (sound_mode == 1) {
    led_data->sound_brows = true;
    led_data->sound_gems = false;
    led_data->sound_fins = true;
  }
  if (sound_mode == 2) {
    led_data->sound_brows = true;
    led_data->sound_gems = true;
    led_data->sound_fins = true;
  }
  if (sound_mode == 3) {
    led_data->sound_brows = false;
    led_data->sound_gems = true;
    led_data->sound_fins = false;
  }

  if (led_data->audio_on == false){
    led_data->sound_brows = false;
    led_data->sound_gems = false;
    led_data->sound_fins = false;
}

  EVERY_N_SECONDS(20) {
    led_data->effect_index++;
  }
  if (led_data->effect_index > 3) {
    led_data->effect_index = 0;
  }
  EVERY_N_SECONDS(11) {
    led_data->palette_index++;
  }
  if (led_data->palette_index > 5) {
    led_data->palette_index = 0;
  }

  int current_mode = effect_order[led_data->effect_index];
  if (led_data->off) current_mode = 255;

  nblendPaletteTowardPalette(palette, palette_order[led_data->palette_index], 24);


  static uint8_t gHue = 0;

  EVERY_N_MILLISECONDS(20) {
    gHue++;
  }
  fadeToBlackBy(led_data->y_leds, Y_LED_NUM, 10);
  fadeToBlackBy(led_data->x_leds, X_LED_NUM, 10);

  if (current_mode == 0) {

    for (int i = 0; i < Y_LED_NUM; i++) {
      led_data->y_leds[i] = ColorFromPalette(palette, gHue + (i * 14), 255);  // span of 14
    }
  }

  if (current_mode == 1) {

    for (int i = 0; i <= X_LED_NUM / 2; i++) {
      led_data->x_leds[i] = ColorFromPalette(palette, (gHue - 32) + (i * 4), 255);  // 32 is an offset to align patterns 4 is the span
    }

    for (int i = 0; i <= X_LED_NUM / 2; i++) {
      led_data->x_leds[44 - i] = led_data->x_leds[i];
    }
  }


  if (current_mode == 2) {  //meh
    // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
    uint8_t BeatsPerMinute = 62;

    uint8_t beat = beatsin8(BeatsPerMinute, 64, 255);
    for (int i = 0; i < X_LED_NUM; i++) {  //9948
      led_data->x_leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
    }
  }

  if (current_mode == 3) {  //cool
    // random colored speckles that blink in and fade smoothly

    int pos = random16(X_LED_NUM);
    led_data->x_leds[pos] += ColorFromPalette(palette, gHue + random8(64), 255);
    for (int i = 0; i <= X_LED_NUM / 2; i++) {
      led_data->x_leds[44 - i] = led_data->x_leds[i];
    }
  }

  //calculate audio stuff
  for (int i = 0; i < 8; i++) {
    led_data->x_fft_leds[i] = ColorFromPalette(palette, gHue + (i * 2), max(cpu_right->fft[i], cpu_left->fft[i]));
    led_data->x_fft_leds[i + 8] = led_data->x_fft_leds[i];
    if (i + 16 <= X_LED_NUM / 2)
      led_data->x_fft_leds[i + 16] = led_data->x_fft_leds[i];
  }

  for (int i = 0; i < 8; i++) {
    led_data->x_fft_leds[44 - i] = led_data->x_fft_leds[i];
    led_data->x_fft_leds[44 - i - 8] = led_data->x_fft_leds[i];
    if (44 - i - 16 >= X_LED_NUM / 2)
      led_data->x_fft_leds[44 - i - 16] = led_data->x_fft_leds[i];
  }
}
