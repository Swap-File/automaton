#include <Arduino.h>
#include "vibe.h"

#define VIBE_SHORT 200
#define VIBE_COOLDOWN 200

uint8_t vibe_read(bool  left, bool right) {
  if (left && right)
    return 0xFF;
  else if (right)
    return 0x0F;
  else if (left)
    return 0xF0;
  return 0x00;
}

void vibe_update(struct cpu_struct *hand_data) {


  if (hand_data->vibe_request > 0) {
    if (millis() - hand_data->vibe_time_event > hand_data->vibe_time_duration) {
      if (hand_data->vibe == false) {
        //start vibing
        hand_data->vibe = true;
        hand_data->vibe_time_event = millis();
        hand_data->vibe_time_duration = VIBE_SHORT;
      } else {
        //stop vibing
        hand_data->vibe_request--;
        hand_data->vibe = false;
        hand_data->vibe_time_event = millis();
        hand_data->vibe_time_duration = VIBE_COOLDOWN;
      }
    }
  }
}
