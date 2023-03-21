#ifndef _COMMON_H
#define _COMMON_H
#include <FastLED.h>

#define FIN_NUM 15
#define LEDS_PER_FIN 3
#define X_LED_NUM (FIN_NUM * LEDS_PER_FIN)
#define Y_LED_NUM 10

struct led_struct {
  CRGB x_leds[X_LED_NUM];      //across the head, 15 fins, 3 leds each
  CRGB x_fft_leds[X_LED_NUM];  //across the head, 15 fins, 3 leds each
  CRGB y_leds[Y_LED_NUM];      //across the head, 15 fins, 3 leds each
  uint8_t servos[FIN_NUM];
  uint8_t helmet_mode;

  //controls
  bool sound_gems = false;
  bool sound_fins = true;
  bool sound_brows = true;
  bool off = false;
  int effect_index = 0;
  int palette_index = 0;

  bool audio_on = true;

 
 
};

#define FFT_SAMPLES 8

struct cpu_struct {
  char id;

  uint32_t gesture_start_time;
  uint8_t gesture_state;
  uint8_t gesture;
  bool gesture_complete_flag;
  bool gesture_start_flag;
  bool gesture_complete_good_flag;
  int vibe_request;
  bool vibe;

  uint32_t vibe_time_duration;
  uint32_t vibe_time_event;

  uint8_t fft[FFT_SAMPLES];

  float yaw_last_gesture;
  bool require_new_yaw;
  float yaw_filtered;
  float yaw_ref;
  uint8_t yaw;


  float pitch_filtered;
  float pitch_ref;
  uint8_t pitch;


  float roll_filtered;
  float roll_ref;
  uint8_t roll;

  uint8_t step_count;

  uint8_t tap_event;
  uint8_t tap_event_counter;

  uint32_t msg_time;
  uint8_t msg_count;

  int fps;
};

#endif
