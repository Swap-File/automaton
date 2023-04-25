#include <Arduino.h>
#include "gesture.h"

static int calc_relative_axis_angle(uint8_t actual_angle, float reference) {
  int angle_diff;
  if (((((int)reference) - actual_angle + 256) % 256) < 128) {
    angle_diff = actual_angle - reference;
    if (angle_diff < -128) {
      angle_diff = angle_diff + 256;
    } else if (angle_diff > 128) {
      angle_diff = angle_diff - 256;
    }
  } else {
    angle_diff = reference - actual_angle;
    if (angle_diff < -128) {
      angle_diff = angle_diff + 256;
    } else if (angle_diff > 128) {
      angle_diff = angle_diff - 256;
    }
    angle_diff = angle_diff * -1;
  }
  return angle_diff;
}

static void print_gesture(uint8_t gesture) {
  if (gesture == GESTURE_CW)
    Serial.print("CW");
  if (gesture == GESTURE_CCW)
    Serial.print("CCW");
  if (gesture == GESTURE_U)
    Serial.print("U");
  if (gesture == GESTURE_D)
    Serial.print("D");
  if (gesture == GESTURE_L)
    Serial.print("L");
  if (gesture == GESTURE_R)
    Serial.print("R");
}


static void set_reference_angles(struct cpu_struct *cpu_data) {
  cpu_data->gesture_start_time = millis();
  cpu_data->gesture_stop_time = 0;
  cpu_data->gesture_found = 0;
  cpu_data->yaw_ref = cpu_data->yaw;
  cpu_data->pitch_ref = cpu_data->pitch;  // allow dynamic start angles
  cpu_data->roll_ref = cpu_data->roll;    // allow dynamic start angles

  //Serial.print(cpu_data->id);
  //Serial.print(" stopped at ");
  //Serial.println(cpu_data->gesture_start_time);
}

void check_for_gestures(struct cpu_struct *cpu_data) {

  if (cpu_data->gesture_found != 0) return;

  int yaw_diff = calc_relative_axis_angle(cpu_data->yaw, cpu_data->yaw_ref);
  if (yaw_diff > YAW_GESTURE)
    cpu_data->gesture_found = GESTURE_L;
  if (yaw_diff < -YAW_GESTURE)
    cpu_data->gesture_found = GESTURE_R;

  if (cpu_data->id != 'H') {  // dont check head for roll and pitch
    int pitch_diff = calc_relative_axis_angle(cpu_data->pitch, cpu_data->pitch_ref);
    int roll_diff = calc_relative_axis_angle(cpu_data->roll, cpu_data->roll_ref);

    // if (roll_diff > ROLL_GESTURE)
    //cpu_data->gesture_found = GESTURE_CW;
    //if (roll_diff < -ROLL_GESTURE)
    //  cpu_data->gesture_found = GESTURE_CCW;
    if (pitch_diff > PITCH_GESTURE)
      cpu_data->gesture_found = GESTURE_U;
    if (pitch_diff < -PITCH_GESTURE)
      cpu_data->gesture_found = GESTURE_D;
  }

  if (cpu_data->gesture_found != 0) {
    cpu_data->gesture_stop_time = millis();
    Serial.print(cpu_data->id);
    Serial.print(" ");
    print_gesture(cpu_data->gesture_found);
    Serial.println(" ");
  }
}

void gesture_start(struct cpu_struct *cpu_left, struct cpu_struct *cpu_right, struct cpu_struct *cpu_head) {
  set_reference_angles(cpu_head);
  set_reference_angles(cpu_right);
  set_reference_angles(cpu_left);
}

void gesture_check(struct cpu_struct *cpu_left, struct cpu_struct *cpu_right, struct cpu_struct *cpu_head) {
  check_for_gestures(cpu_left);
  check_for_gestures(cpu_right);
  check_for_gestures(cpu_head);
}
