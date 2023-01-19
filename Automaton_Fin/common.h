#ifndef _COMMON_H
#define _COMMON_H

#define FFT_SAMPLES 8


struct CRGB {
  union {
    struct {
      union {
        uint8_t r;
        uint8_t red;
      };
      union {
        uint8_t g;
        uint8_t green;
      };
      union {
        uint8_t b;
        uint8_t blue;
      };
    };
    uint8_t raw[3];
  };
};


struct fin_struct {

  CRGB strip[3];
  CRGB led;
  uint8_t servo;

};

#endif
