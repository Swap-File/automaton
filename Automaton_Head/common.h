#ifndef _COMMON_H
#define _COMMON_H

#define FFT_SAMPLES 8


#define GESTURE_CW   0b00000001
#define GESTURE_CCW  0b00000010
#define GESTURE_C    0b00111111
#define GESTURE_L    0b00000100
#define GESTURE_R    0b00001000
#define GESTURE_LR   0b00111100
#define GESTURE_U    0b00010000
#define GESTURE_D    0b00100000
#define GESTURE_UD   0b00110000
#define GESTURE_NO   0b11000000

struct cpu_struct {
  
  int gesture_magnitude_last; 
  int gesture_magnitude; 
  bool gesture_handled;
  uint8_t gesture;

  int vibe_request;
  bool vibe;

  uint32_t vibe_time_duration;
  uint32_t vibe_time_event;

  uint8_t fft[FFT_SAMPLES];

  int centered_counter;
  uint32_t centered_time;

  float   yaw_filtered;
  float   yaw_ref;
  uint8_t yaw;

  float   pitch_filtered;
  float   pitch_ref;
  uint8_t pitch;

  float   roll_filtered;
  float   roll_ref;
  uint8_t roll;

  uint8_t step_count;

  uint8_t tap_event;
  uint8_t tap_event_counter;

  uint32_t msg_time;
  uint8_t msg_count;

  int fps;

};

#endif
