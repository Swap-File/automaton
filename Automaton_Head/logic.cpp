#include <Arduino.h>
#include "common.h"
#include "fin.h"
#include "gesture.h"
#include "ble.h"
#include "vibe.h"


void logic_update(struct led_struct *led_data, struct cpu_struct *cpu_left, struct cpu_struct *cpu_right, struct cpu_struct *cpu_head) {

  static bool fin_motion_from_hands = false;    //hand reaction movement
  static bool fin_motion_from_walking = false;  //walking reaction movement

  static bool fin_animation = false;
  static uint32_t fin_animation_start_time = 0;
  static uint8_t fin_animation_effect = FIN_ALT;


  static uint32_t tap_time = 0;
  static bool gesture_check_enabled = false;

  static uint32_t tap_cooldown = 0;  //prevent left and right hand both triggering when clapping

  static uint32_t head_tap_cooldown = 0;  // prevent self tap from servo slap
  static int last_fin_state = -1;

  if (last_fin_state != fin_mode()) {
    last_fin_state = fin_mode();
    head_tap_cooldown = millis() + 2000;
  }

  int event = 0;

  if (cpu_head->tap_event_counter != cpu_head->tap_event_counter_previous) {
    cpu_head->tap_event_counter_previous = cpu_head->tap_event_counter;
    Serial.print("cpu_head tap ");
    Serial.println(cpu_head->tap_event);
    if (millis() < tap_cooldown || millis() < head_tap_cooldown) {
      Serial.println("cooldown - ignoring");
    } else {
      event = cpu_head->tap_event;
      tap_cooldown = millis() + 300;
    }
  }

  if (cpu_left->tap_event_counter != cpu_left->tap_event_counter_previous) {
    cpu_left->tap_event_counter_previous = cpu_left->tap_event_counter;
    Serial.print("cpu_left tap ");
    Serial.println(cpu_left->tap_event);
    if (millis() < tap_cooldown) {
      Serial.println("cooldown - ignoring");
    } else {
      event = cpu_left->tap_event;
      tap_cooldown = millis() + 300;
    }
  }

  if (cpu_right->tap_event_counter != cpu_right->tap_event_counter_previous) {
    cpu_right->tap_event_counter_previous = cpu_right->tap_event_counter;
    Serial.print("cpu_right tap ");
    Serial.println(cpu_right->tap_event);
    if (millis() < tap_cooldown) {
      Serial.println("cooldown - ignoring");
    } else {
      event = cpu_right->tap_event;
      tap_cooldown = millis() + 300;
    }
  }



  if (event) {

    gesture_start(cpu_left, cpu_right, cpu_head);

    cpu_left->vibe_request = event;
    cpu_right->vibe_request = event;


    if (event == 1) {
      tap_time = millis();
      gesture_check_enabled = true;
    }

    if (event == 2) {  //turn on, without sound
      led_data->off = false;
      fin_set(FIN_MID, FIN_ALT);
      fin_animation_start_time = millis();
      fin_animation_effect = FIN_ALT;
      fin_animation = true;

      fin_motion_from_walking = false;
      led_data->audio_on = false;
    }
    if (event == 3) {  //turn off
      fin_set(FIN_DOWN, FIN_ALT);
      fin_motion_from_walking = false;
      led_data->off = true;
    }
    if (event == 4) {  //walking auto mde
      Serial.println(" enable auto mode");
      fin_set(FIN_MID, FIN_ALT);
      fin_motion_from_walking = true;
      led_data->off = false;
    }
  }

  if (millis() < tap_time + 1000 && gesture_check_enabled) {
    gesture_check(cpu_left, cpu_right, cpu_head);

    if (cpu_head->gesture_found == GESTURE_L) {
      gesture_check_enabled = false;
      led_data->audio_on = true;
      fin_set(FIN_MID, FIN_LEFT);
      fin_animation_start_time = millis();
      fin_animation_effect = FIN_LEFT;
      fin_animation = true;
      cpu_left->vibe_request = 1;
      cpu_right->vibe_request = 1;
    } else if (cpu_head->gesture_found == GESTURE_R) {
      gesture_check_enabled = false;
      led_data->audio_on = false;
      fin_set(FIN_MID, FIN_RIGHT);
      fin_animation_start_time = millis();
      fin_animation_effect = FIN_RIGHT;
      fin_animation = true;
      cpu_left->vibe_request = 1;
      cpu_right->vibe_request = 1;
    } else if (cpu_left->gesture_found == GESTURE_U || cpu_right->gesture_found == GESTURE_U) {
      Serial.println("hand up");


      gesture_check_enabled = false;

      fin_motion_from_hands = true;

      fin_set(FIN_MID, FIN_LEFT);
      fin_animation_start_time = millis();
      fin_animation_effect = FIN_LEFT;
      fin_animation = true;
      cpu_left->vibe_request = 1;
      cpu_right->vibe_request = 1;

    } else if (cpu_left->gesture_found == GESTURE_D || cpu_right->gesture_found == GESTURE_D) {
      Serial.println("hand down");
      gesture_check_enabled = false;

      fin_motion_from_hands = false;

      fin_set(FIN_MID, FIN_RIGHT);
      fin_animation_start_time = millis();
      fin_animation_effect = FIN_RIGHT;
      fin_animation = true;
      cpu_left->vibe_request = 1;
      cpu_right->vibe_request = 1;
    }
  }


  // fin motion from walking
  if (fin_motion_from_walking) {

    static int step_combo = 0;
    static uint8_t step_count_last = 0;
    static uint32_t step_count_time = 0;

    if (cpu_head->step_count != step_count_last) {
      step_count_last = cpu_head->step_count;
      if (step_count_time + 1000 > millis()) {
        step_combo++;
      } else {
        step_combo = 0;
      }
      step_count_time = millis();
      Serial.println("step number ");
      Serial.println(step_combo);
      if (step_combo > 10) {
        Serial.println("Doing a random cool");

        if (fin_random() == FIN_DOWN) {
          fin_animation = true;
          fin_animation_start_time = millis();
        }
        step_combo = 0;
      }
    }
  }

  //fin animation (delayed)
  if (fin_animation && millis() > fin_animation_start_time + 1000) {
    fin_animation = false;
    fin_set(FIN_UP, fin_animation_effect);
  }
  // fin motion from hands
  static int last_location_left = -1;
  static int last_location_right = -1;
  static uint32_t hand_time = 0;

#define FIN_COOLDOWN_HAND 300
#define SIDEWAYS 20  // 128  170

  if (fin_motion_from_hands) {
    if (ble_connection_count() > 0) {  //fin_mode
      if (cpu_left->pitch_smoothed > 127 + PITCH_GESTURE && last_location_left != FIN_DOWN) {
        if (cpu_left->roll_smoothed < 127 - SIDEWAYS) {
          if (fin_mode() == FIN_DOWN) {
            if (millis() > hand_time + FIN_COOLDOWN_HAND)
              fin_bump(1, -20);  // down
          } else {
            fin_set(FIN_DOWN, FIN_LEFT);
            hand_time = millis();
          }
        } else {
          if (fin_mode() == FIN_UP) {
            if (millis() > hand_time + FIN_COOLDOWN_HAND)
              fin_bump(1, 20);  // down
          } else {
            fin_set(FIN_UP, FIN_LEFT);
            hand_time = millis();
          }
        }

        last_location_left = FIN_DOWN;


      } else if (cpu_left->pitch_smoothed < 127 - PITCH_GESTURE && last_location_left != FIN_UP) {
        if (fin_mode() == FIN_UP) {
          if (millis() > hand_time + FIN_COOLDOWN_HAND)
            fin_bump(1, 20);  // down
        } else {
          fin_set(FIN_UP, FIN_LEFT);
          hand_time = millis();
        }
        last_location_left = FIN_UP;

      } else if (cpu_left->pitch_smoothed > 127 - PITCH_GESTURE && cpu_left->pitch_smoothed < 127 + PITCH_GESTURE && last_location_left != FIN_MID) {
        if (fin_mode() == FIN_MID) {
          if (millis() > hand_time + FIN_COOLDOWN_HAND)
            fin_bump(1, 20);  // down
        } else {
          fin_set(FIN_MID, FIN_LEFT);
          hand_time = millis();
        }
        last_location_left = FIN_MID;
      }

      if (cpu_right->pitch_smoothed > 127 + PITCH_GESTURE && last_location_right != FIN_DOWN) {

        if (cpu_right->roll_smoothed > 127 + SIDEWAYS) {


          if (fin_mode() == FIN_DOWN) {
            if (millis() > hand_time + FIN_COOLDOWN_HAND)
              fin_bump(-1, -20);  //left down
          } else {
            fin_set(FIN_DOWN, FIN_RIGHT);
            hand_time = millis();
          }
        } else {

          if (fin_mode() == FIN_UP) {
            if (millis() > hand_time + FIN_COOLDOWN_HAND)
              fin_bump(-1, 20);  //left down
          } else {
            fin_set(FIN_UP, FIN_RIGHT);
            hand_time = millis();
          }
        }


        last_location_right = FIN_DOWN;

      } else if (cpu_right->pitch_smoothed < 127 - PITCH_GESTURE && last_location_right != FIN_UP) {
        if (fin_mode() == FIN_UP) {
          if (millis() > hand_time + FIN_COOLDOWN_HAND)
            fin_bump(-1, 20);  //left down
        } else {
          fin_set(FIN_UP, FIN_RIGHT);
          hand_time = millis();
        }

        last_location_right = FIN_UP;

      } else if (cpu_right->pitch_smoothed < 127 + PITCH_GESTURE && cpu_right->pitch_smoothed > 127 - PITCH_GESTURE && last_location_right != FIN_MID) {
        if (fin_mode() == FIN_MID) {
          if (millis() > hand_time + FIN_COOLDOWN_HAND)
            fin_bump(-1, 20);  //left down
        } else {
          fin_set(FIN_MID, FIN_RIGHT);
          hand_time = millis();
        }

        last_location_right = FIN_MID;
      }

    } else {
      last_location_left = -1;
      last_location_right = -1;
    }
  }

  vibe_update(cpu_left);
  vibe_update(cpu_right);
}
