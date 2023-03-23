#include <Arduino.h>
#include "common.h"
#include "fin.h"

// from fin.cpp
extern uint8_t servo_mode;
extern uint8_t fin_effect;

// from ble.cpp
extern uint8_t connection_count;

void logic_update(struct led_struct *led_data, struct cpu_struct *cpu_left, struct cpu_struct *cpu_right, struct cpu_struct *cpu_head) {
  static uint8_t connection_count_last = 255;

  //disconnected audio force to animation only
  if (connection_count == 0 && connection_count_last != connection_count) {
  //  led_data->audio_on = false;
    Serial.println("No BT connection, forcing audio mode off...");
    connection_count_last = connection_count;
  }

  static bool walking_fin_auto = false;
  static bool head_ani = false;
  static uint32_t head_ani_tap_time = 0;

  if (cpu_head->tap_event_counter != cpu_head->tap_event_counter_previous) {
    cpu_head->tap_event_counter_previous = cpu_head->tap_event_counter;
    Serial.print("cpu_head tap ");
    Serial.println(cpu_head->tap_event);

    if (cpu_head->tap_event == 2) {
      led_data->off = false;
      servo_mode = FIN_MID;
      fin_effect = FIN_ALT;
      head_ani_tap_time = millis();
      head_ani = true;
      walking_fin_auto = false;
    }
    if (cpu_head->tap_event == 3) {
      servo_mode = FIN_DOWN;
      fin_effect = FIN_ALT;
      walking_fin_auto = false;
      led_data->off = true;
    }
    if (cpu_head->tap_event == 4) {
      Serial.println(" enable auto mode");
      servo_mode = FIN_MID;
      fin_effect = FIN_ALT;
      walking_fin_auto = true;
      led_data->off = false;
    }
  }




  if (walking_fin_auto) {

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

        int last_mode = servo_mode;
        int last_effect = fin_effect;

        while (last_mode == servo_mode) {
          servo_mode = random(FIN_MODE_FIRST, FIN_MODE_LAST);
        }
        while (last_effect == fin_effect) {
          fin_effect = random(FIN_EFFECT_FIRST, FIN_EFFECT_LAST);
        }
        if (servo_mode == FIN_DOWN) {
          head_ani = true;
          head_ani_tap_time = millis();
        }

        step_combo = 0;
      }
    }
  }

  if (head_ani && millis() > head_ani_tap_time + 1000) {
    head_ani = false;
    servo_mode = FIN_UP;
  }


  if (cpu_left->tap_event_counter != cpu_left->tap_event_counter_previous) {
    cpu_left->tap_event_counter_previous = cpu_left->tap_event_counter;
    Serial.print("cpu_left tap ");
    Serial.println(cpu_left->tap_event);
  }

  if (cpu_right->tap_event_counter != cpu_right->tap_event_counter_previous) {
    cpu_right->tap_event_counter_previous = cpu_right->tap_event_counter;
    Serial.print("cpu_right tap ");
    Serial.println(cpu_right->tap_event);
  }
}
