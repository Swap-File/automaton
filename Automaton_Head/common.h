#ifndef _COMMON_H
#define _COMMON_H

#define FFT_SAMPLES 8

struct cpu_struct {
  bool vibe;
  
  uint8_t fft[FFT_SAMPLES];

  uint8_t yaw;
  uint8_t pitch;
  uint8_t roll;

  uint8_t step_count;

  uint8_t tap_event;
  uint8_t tap_event_counter;

  uint32_t msg_time;
  uint8_t msg_count;

  int fps;

  bool connected;
};


#endif
