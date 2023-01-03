#include <Arduino.h>

#define vibe_short 200
#define vibe_long 1000

#define vibe_short_pause 200
#define vibe_long_pause 1000


int vibe_left[256] = {0};
uint8_t vibe_left_write_idx = 0;
uint8_t vibe_left_read_idx = 0;

int vibe_right[256] = {0};
uint8_t vibe_right_write_idx = 0;
uint8_t vibe_right_read_idx = 0;

bool vibe_left_active = false;
bool vibe_right_active = false;

uint8_t vibe_read(bool  left, bool right) {
  if (left && right)
    return 0xFF;
  else if (right)
    return 0x0F;
  else if (left)
    return 0xF0;
  return 0x00;
}

void vibe_update(bool * left, bool *right) {

  static uint32_t left_vibe_time = 0;
  static uint32_t left_vibe_length = 0;

  if (vibe_left_read_idx != vibe_left_write_idx) {
    if (millis() - left_vibe_time > left_vibe_length) {
      left_vibe_time = millis();
      left_vibe_length = vibe_left[vibe_left_read_idx++];
      if (vibe_left_read_idx % 2 == 1) {
        *left = true;
      } else {
        *left = false;
      }
    }
  }

  static uint32_t right_vibe_time = 0;
  static uint32_t right_vibe_length = 0;

  if (vibe_right_read_idx != vibe_right_write_idx) {
    if (millis() - right_vibe_time > right_vibe_length) {
      right_vibe_time = millis();
      right_vibe_length = vibe_right[vibe_right_read_idx++];
      if (vibe_right_read_idx % 2 == 1) {
        *right = true;
      } else {
        *right = false;
      }
    }
  }
}

void vibe_add_quick_slow(bool side, int quick, int slow) {
  if (side == 0) {
    for (int i = 0; i < quick; i++) {
      vibe_left[vibe_left_write_idx++] = vibe_short;
      vibe_left[vibe_left_write_idx++] = vibe_short_pause;
    }
    vibe_left[vibe_left_write_idx - 1] = vibe_long_pause;

    for (int i = 0; i < slow; i++) {
      vibe_left[vibe_left_write_idx++] = vibe_long;
      vibe_left[vibe_left_write_idx++] = vibe_short_pause;
    }
    vibe_left[vibe_left_write_idx - 1] = vibe_long_pause;
  }

  if (side == 1) {
    for (int i = 0; i < quick; i++) {
      vibe_right[vibe_right_write_idx++] = vibe_short;
      vibe_right[vibe_right_write_idx++] = vibe_short_pause;
    }
    vibe_right[vibe_right_write_idx - 1] = vibe_long_pause;

    for (int i = 0; i < slow; i++) {
      vibe_right[vibe_right_write_idx++] = vibe_long;
      vibe_right[vibe_right_write_idx++] = vibe_short_pause;
    }
    vibe_right[vibe_right_write_idx - 1] = vibe_long_pause;
  }

}
