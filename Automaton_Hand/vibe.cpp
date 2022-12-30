#include <Arduino.h>

#define vibe_short 200
#define vibe_long 1000

#define vibe_short_pause 200
#define vibe_long_pause 1000

int vibes[256] = {0};
uint8_t write_idx = 0;
uint8_t read_idx = 0;

void vibe_init(void) {

  pinMode(D0, OUTPUT);
  pinMode(D0, LOW);

}

bool vibe_active(void) {
  if (write_idx != read_idx)
    return true;
  else
    return false;
}

void vibe_update(void) {
  static uint32_t vibe_time = 0;
  static uint32_t vibe_length = 0;
  if (read_idx != write_idx) {
    if (millis() - vibe_time > vibe_length) {
      vibe_time = millis();
      vibe_length = vibes[read_idx++];
      if (read_idx % 2 == 1) {
        pinMode(D0, HIGH);
      } else {
        pinMode(D0, LOW);
      }
    }
  }
}

void vibe_add_quick_slow(int quick, int slow) {
  for (int i = 0; i < quick; i++) {
    vibes[write_idx++] = vibe_short;
    vibes[write_idx++] = vibe_short_pause;
  }
  vibes[write_idx - 1] = vibe_long_pause;

  for (int i = 0; i < slow; i++) {
    vibes[write_idx++] = vibe_long;
    vibes[write_idx++] = vibe_short_pause;
  }
  vibes[write_idx - 1] = vibe_long_pause;
}
