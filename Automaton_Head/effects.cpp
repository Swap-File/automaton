#include <Arduino.h>
#include <FastLED.h>

#include "common.h"

int effect_mode = 8;

void effects_update(struct led_struct *led_data, const struct cpu_struct *cpu_left, const struct cpu_struct *cpu_right) {

  static uint8_t gHue = 0;

  EVERY_N_MILLISECONDS(20) {
    gHue++;
  }

  static int blocker = 0;
  if (effect_mode == 0) {
    fill_rainbow(led_data->y_leds, Y_LED_NUM, gHue, 14);
  }

  if (effect_mode == 1) {
    fill_rainbow(led_data->x_leds, X_LED_NUM, gHue, 7);

    for (int i = 0; i < 22; i++) {
      led_data->x_leds[44 - i] = led_data->x_leds[i];
    }
  }

  if (effect_mode == 2) {
    // eight colored dots, weaving in and out of sync with each other
    fadeToBlackBy(led_data->x_leds, X_LED_NUM, 20);
    uint8_t dothue = 0;
    for (int i = 0; i < 8; i++) {
      led_data->x_leds[beatsin16(i + 7, 0, X_LED_NUM - 1)] |= CHSV(dothue, 200, 255);
      dothue += 32;
    }
  }

  if (effect_mode == 3) {
    // a colored dot sweeping back and forth, with fading trails
    fadeToBlackBy(led_data->x_leds, X_LED_NUM, 20);
    int pos = beatsin16(13, 0, X_LED_NUM - 1);
    led_data->x_leds[pos] += CHSV(gHue, 255, 192);
  }

  if (effect_mode == 4) {
    // a colored dot sweeping back and forth, with fading trails
    fadeToBlackBy(led_data->y_leds, Y_LED_NUM, 20);
    int pos = beatsin16(13, 0, Y_LED_NUM - 1);
    led_data->y_leds[pos] += CHSV(gHue, 255, 192);
  }

  if (effect_mode == 5) {
    // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
    uint8_t BeatsPerMinute = 62;
    CRGBPalette16 palette = PartyColors_p;
    uint8_t beat = beatsin8(BeatsPerMinute, 64, 255);
    for (int i = 0; i < X_LED_NUM; i++) {  //9948
      led_data->x_leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
    }
  }

  if (effect_mode == 6) {
    // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
    uint8_t BeatsPerMinute = 30;
    CRGBPalette16 palette = PartyColors_p;
    uint8_t beat = beatsin8(BeatsPerMinute, 64, 255);
    for (int i = 0; i < Y_LED_NUM; i++) {  //9948
      led_data->y_leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
    }
  }

  if (effect_mode == 7) {
    // random colored speckles that blink in and fade smoothly
    fadeToBlackBy(led_data->x_leds, X_LED_NUM, 10);
    int pos = random16(X_LED_NUM);
    led_data->x_leds[pos] += CHSV(gHue + random8(64), 200, 255);
  }

  if (effect_mode == 8) {
    //led test
    CRGBPalette16 palette = PartyColors_p;

    for (int i = 0; i < 8; i++) {
      // led_data->x_leds[i] = CRGB(cpu_left->fft[i], cpu_right->fft[i], 0);
      // led_data->x_leds[16 - 1 - i] = CRGB(cpu_left->fft[i], cpu_right->fft[i], 0);
      led_data->x_leds[i] = ColorFromPalette(palette, gHue + (i * 2), max(cpu_right->fft[i], cpu_left->fft[i]));
      led_data->x_leds[i + 8] = led_data->x_leds[i];
      if (i + 16 <= X_LED_NUM / 2)
        led_data->x_leds[i + 16] = led_data->x_leds[i];
    }

    for (int i = 0; i < 8; i++) {
      led_data->x_leds[44 - i] = led_data->x_leds[i];
      led_data->x_leds[44 - i - 8] = led_data->x_leds[i];
      if (44 - i - 16 >= X_LED_NUM / 2)
        led_data->x_leds[44 - i - 16] = led_data->x_leds[i];
    }
  }
}
