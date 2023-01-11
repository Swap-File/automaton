#ifndef _COMMON_H
#define _COMMON_H

#define FFT_SAMPLES 8



struct cpu_struct {
  char id;

  uint32_t gesture_idled_time;
  int gesture_magnitude_last; 
  int gesture_magnitude; 
  uint8_t gesture_state;
  uint8_t gesture;
  bool gesture_completed;
  bool gesture_idled;

  int vibe_request;
  bool vibe;

  uint32_t vibe_time_duration;
  uint32_t vibe_time_event;

  uint8_t fft[FFT_SAMPLES];


  float   yaw_last_gesture;
  bool    check_yaw;
  
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
