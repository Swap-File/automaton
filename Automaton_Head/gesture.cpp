#include <Arduino.h>
#include "gesture.h"
#include "vibe.h"

static int calc_relative_axis_angle(uint8_t actual_angle, float reference) {
  int angle_diff;
  if (((((int)reference) - actual_angle + 256) % 256) < 128) {
    angle_diff = actual_angle - reference;
    if (angle_diff < -128) {
      angle_diff = angle_diff + 256;
    } else if (angle_diff > 128) {
      angle_diff = angle_diff - 256;
    }
  }
  else {
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


static int calc_relative_axis_angle(uint8_t actual_angle, float * reference, float filter) {
  int angle_diff = calc_relative_axis_angle(actual_angle, *reference);
  * reference += filter * angle_diff;
  if ( *reference > 255)
    *reference -= 255;
  if ( *reference < 0)
    *reference += 255;
  return angle_diff;
}


static void print_gesture(uint8_t gesture ) {
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

static void check_for_gestures(struct cpu_struct *cpu_data) {
  if (cpu_data->gesture_state != GESTURE_RECOGNIZING)
    return;

  //priority of gestures:  up down, left right, then cc ccw

  int yaw_diff = calc_relative_axis_angle(cpu_data->yaw, cpu_data->yaw_ref);
  int pitch_diff = calc_relative_axis_angle(cpu_data->pitch, cpu_data->pitch_ref);
  int roll_diff = calc_relative_axis_angle(cpu_data->roll, cpu_data->roll_ref);

  if ( cpu_data->gesture_magnitude > cpu_data->gesture_magnitude_last) {
    //Serial.println(" Mag increase");
    if ((cpu_data->gesture & GESTURE_C) == 0x00) {
      if ( roll_diff > ROLL_GESTURE)
        cpu_data->gesture = GESTURE_CW;
      if ( roll_diff  < -ROLL_GESTURE)
        cpu_data->gesture = GESTURE_CCW;
    }

    if ((cpu_data->gesture & GESTURE_LR) == 0x00) {
      if ( yaw_diff > YAW_GESTURE)
        cpu_data->gesture = GESTURE_L;
      if ( yaw_diff  < -YAW_GESTURE)
        cpu_data->gesture = GESTURE_R;
    }

    if ((cpu_data->gesture & GESTURE_UD) == 0x00) {
      if ( pitch_diff > PITCH_GESTURE)
        cpu_data->gesture = GESTURE_U;
      if ( pitch_diff  < -PITCH_GESTURE)
        cpu_data->gesture = GESTURE_D;
    }
  } else {  // once the the magnitude is no longer increasing, process it
    if ( cpu_data->gesture ) {

      cpu_data->gesture_completed = true;

      cpu_data-> yaw_last_gesture = cpu_data-> yaw;
      cpu_data-> roll_last_gesture = cpu_data->roll;
      if ((cpu_data->gesture & (GESTURE_L | GESTURE_R )) != 0x00)
        cpu_data->require_new_yaw = true;
      else
        cpu_data->require_new_yaw = false;

      cpu_data->gesture_state = GESTURE_IDLE;
      //Serial.print(" ");
      //Serial.print(cpu_data->id);
      //Serial.println(" gesture complete ");
      //print_gesture(cpu_data->gesture);
      cpu_data->gesture_idled = false;
    }
  }

  cpu_data->gesture_magnitude_last = cpu_data->gesture_magnitude;
}

static void set_reference_angles(struct cpu_struct *cpu_data) {
  cpu_data->gesture_idled_time = millis();
  cpu_data->gesture = 0;
  cpu_data->yaw_ref = cpu_data->yaw;
  cpu_data->pitch_ref = 128; // fixed start angle of straight ahead
  cpu_data->roll_ref = 128;  // fixed start angle of level roll
  //cpu_data->pitch_ref = cpu_data->pitch; // allow dynamic start angles
  //cpu_data->roll_ref = cpu_data->roll;   // allow dynamic start angles

  //Serial.print(cpu_data->id);
  //Serial.print(" stopped at ");
  //Serial.println(cpu_data->gesture_idled_time);

}

static inline bool check_absolute(uint8_t angle, int deadzone)  {
  if (angle > 128 - deadzone && angle < 128 + deadzone)
    return true;
  return false;
}

static inline bool check_relative(uint8_t angle, float angle_last_gesture, int deadzone) {
  int angle_diff = calc_relative_axis_angle(angle, angle_last_gesture);
  if (abs(angle_diff) > deadzone )
    return true;
  return false;
}

bool check_idle(struct cpu_struct *cpu_data) {
  if (!check_absolute(cpu_data->pitch , PITCH_DEADZONE)) {
    cpu_data->require_new_yaw = false;
    return false;
  }
  if (!check_absolute(cpu_data->roll, ROLL_DEADZONE )) {
    cpu_data->require_new_yaw = false;
    return false;
  }
  if (cpu_data->require_new_yaw) {
    if (!check_relative(cpu_data->yaw, cpu_data->yaw_last_gesture, YAW_DEADZONE)) // this must be last, order matters
      return false;
  }
  return true;
}

static void check_for_idle(struct cpu_struct *cpu_data) {

  int yaw_diff = calc_relative_axis_angle(cpu_data->yaw, &(cpu_data->yaw_filtered), 0.2);
  int pitch_diff = calc_relative_axis_angle(cpu_data->pitch, &(cpu_data->pitch_filtered), 0.2);
  int roll_diff = calc_relative_axis_angle(cpu_data->roll, &(cpu_data->roll_filtered), 0.2);

  cpu_data->gesture_magnitude = abs(roll_diff) + abs(pitch_diff) + abs(yaw_diff);

  if ( abs(roll_diff) < 3 && abs(pitch_diff) < 3 && abs(yaw_diff) < 3 && check_idle(cpu_data) ) {

    if (cpu_data->gesture_state < GESTURE_IDLE_COUNT) {
      cpu_data->gesture_completed = false;
      cpu_data->gesture_state++;
      //Serial.print(cpu_data->id);
      //Serial.print(" idle ");
      //Serial.println(cpu_data->gesture_state);

      if (cpu_data->gesture_state == GESTURE_IDLE_COUNT ) {
        set_reference_angles(cpu_data);
        cpu_data->gesture_idled = true;
        //cpu_data->vibe_request = 1;  //debug only
        cpu_data->gesture_state = GESTURE_RECOGNIZING;
      }
    }
  }

  if ( cpu_data->gesture_state == GESTURE_RECOGNIZING) {
    if (millis() - cpu_data->gesture_idled_time > GESTURE_TIMEOUT) {
      cpu_data->gesture_state = GESTURE_IDLE;
      //Serial.print(cpu_data->id);
      //Serial.println(" GESTURE_TIMEOUT");
      cpu_data->yaw_last_gesture = cpu_data->yaw;
      cpu_data->roll_last_gesture = cpu_data->roll;
      cpu_data->gesture_completed = true;
      cpu_data->gesture_idled = false;
    }
  }
}


void gesture_check(struct cpu_struct * cpu_left, struct cpu_struct * cpu_right, struct cpu_struct * cpu_head) {
  check_for_idle(cpu_left);
  check_for_idle(cpu_right);

  check_for_gestures(cpu_left);
  check_for_gestures(cpu_right);
  check_for_gestures(cpu_head);


  if (cpu_left->gesture_idled && cpu_right->gesture_idled) {
    if (abs((int64_t)cpu_left->gesture_idled_time - (int64_t)cpu_right->gesture_idled_time) < ALLOWABLE_IDLE_TIME_VARIANCE) {
      cpu_left->gesture_idled = false;
      cpu_right->gesture_idled = false;

      cpu_left->vibe_request = 1;
      cpu_right->vibe_request = 1;
      // Lock head position here?
      //set_reference_angles(cpu_head);
      //cpu_head->gesture_state = GESTURE_RECOGNIZING;
      //Serial.println("Both Arms Stopped together");
    }
  }

  if ((cpu_left->gesture_completed && cpu_right->gesture_completed) || cpu_head->gesture_completed) { // check for a head gesture?
    cpu_left->gesture_completed = false;
    cpu_right->gesture_completed = false;
    cpu_head->gesture_completed = false;

    Serial.print("Gesture: \t");
    print_gesture(cpu_left->gesture);
    Serial.print("\t");
    print_gesture(cpu_right->gesture);
    Serial.print("\t");
    print_gesture(cpu_head->gesture);
    Serial.println(" ");

    if (cpu_left->gesture != 0 && cpu_right->gesture != 0) {
      cpu_left->vibe_request = 1;
      cpu_right->vibe_request = 1;
    }
  }

  vibe_update(cpu_left);
  vibe_update(cpu_right);


}
