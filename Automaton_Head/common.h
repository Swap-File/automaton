#ifndef _COMMON_H
#define _COMMON_H

#define FFT_SAMPLES 8

struct cpu_struct {
  bool vibe;

  uint8_t fft[FFT_SAMPLES];

  uint8_t cpu_data;

  int recenter_counter;
  
  float   yaw_filtered;
  float     yaw_ref;
  uint8_t yaw;

  float   pitch_filtered;
  float     pitch_ref;
  uint8_t pitch;

  float   roll_filtered;
  float     roll_ref;
  uint8_t roll;

  uint8_t step_count;

  uint8_t tap_event;
  uint8_t tap_event_counter;

  uint32_t msg_time;
  uint8_t msg_count;

  int fps;

};

#endif
