#include <Arduino.h>
#include "gesture.h"
#include "vibe.h"

bool check_yaw = false;

static int calc_relative_axis_angle(uint8_t actual_angle, float * reference, float filter) {
  int angle_diff;
  if (((((int)*reference) - actual_angle + 256) % 256) < 128) {
    angle_diff = actual_angle - *reference;
    if (angle_diff < -128) {
      angle_diff = angle_diff + 256;
    } else if (angle_diff > 128) {
      angle_diff = angle_diff - 256;
    }
  }
  else {
    angle_diff = *reference - actual_angle;
    if (angle_diff < -128) {
      angle_diff = angle_diff + 256;
    } else if (angle_diff > 128) {
      angle_diff = angle_diff - 256;
    }
    angle_diff = angle_diff * -1;
  }
  *reference += filter * angle_diff;  //adjust smoothing here
  if ( *reference > 255)
    *reference -= 255;
  if ( *reference < 0)
    *reference += 255;
  return angle_diff;
}

static inline bool check_pitch(struct cpu_struct *cpu_data) {
  if (cpu_data->pitch > 128 - PITCH_DEADZONE && cpu_data->pitch < 128 + PITCH_DEADZONE)
    return true;
  check_yaw = false;
  return false;
}

static inline bool check_roll(struct cpu_struct *cpu_data) {
  if (cpu_data->roll > 128 - ROLL_DEADZONE && cpu_data->roll < 128 + ROLL_DEADZONE)
    return true;
  check_yaw = false;
  return false;
}

static inline bool check_yaw2(struct cpu_struct *cpu_data) {   // to do
  if (check_yaw == false)
    return true;

  int yaw_diff = calc_relative_axis_angle(cpu_data->yaw, &(cpu_data->yaw_last_gesture), 0.0);

  if (abs(yaw_diff) > YAW_DEADZONE)
    return true;
  return false;
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

  //priority up down, left right, then cc ccw
  int yaw_diff = calc_relative_axis_angle(cpu_data->yaw, &(cpu_data->yaw_ref), 0.0);
  int pitch_diff = calc_relative_axis_angle(cpu_data->pitch, &(cpu_data->pitch_ref), 0.0);
  int roll_diff = calc_relative_axis_angle(cpu_data->roll, &(cpu_data->roll_ref), 0.0);

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
  } else {  //now that the magnitude is no longer in progress, process it
    if ( cpu_data->gesture ) {

      cpu_data->gesture_completed = true;

      cpu_data-> yaw_last_gesture = cpu_data-> yaw;
      cpu_data->  roll_last_gesture = cpu_data->roll;
      if ((cpu_data->gesture & (GESTURE_L | GESTURE_R )) != 0x00)
        check_yaw = true;
      else
        check_yaw = false;

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

static void set_angle_reference(struct cpu_struct *cpu_data) {
  cpu_data->gesture_idled_time = millis();
  cpu_data->gesture = 0;
  cpu_data->yaw_ref = cpu_data->yaw;
  //cpu_data->pitch_ref = cpu_data->pitch; //allow dynamic start angles
  //cpu_data->roll_ref = cpu_data->roll;   //allow dynamic start angles
  cpu_data->pitch_ref = 128;
  cpu_data->roll_ref = 128;

  //Serial.print(cpu_data->id);
  //Serial.print(" stopped at ");
  //Serial.println(cpu_data->gesture_idled_time);

}

bool check_idle(struct cpu_struct *cpu_data) {
  if (!check_pitch(cpu_data))
    return false;
  if (!check_roll(cpu_data))
    return false;
  if (!check_yaw2(cpu_data))  // this must be last, order matters
    return false;
  return true;
}

static void find_relative_angles(struct cpu_struct *cpu_data) {

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
        set_angle_reference(cpu_data);
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
      cpu_data-> yaw_last_gesture = cpu_data->yaw;
      cpu_data->  roll_last_gesture = cpu_data->roll;
      check_yaw = true;
      cpu_data->gesture_completed = true;
      cpu_data->gesture_idled = false;
    }
  }
}


void gesture_check(struct cpu_struct * cpu_left, struct cpu_struct * cpu_right, struct cpu_struct * cpu_head) {
  find_relative_angles(cpu_left);
  find_relative_angles(cpu_right);
  //find_relative_angles(cpu_head);

  check_for_gestures(cpu_left);
  check_for_gestures(cpu_right);
  //check_for_gestures(cpu_head);


  if (cpu_left->gesture_idled && cpu_right->gesture_idled) {
    if (abs((int64_t)cpu_left->gesture_idled_time - (int64_t)cpu_right->gesture_idled_time) < IDLE_TIME_VARIANCE) {
      cpu_left->gesture_idled = false;
      cpu_right->gesture_idled = false;

      cpu_left->vibe_request = 1;
      cpu_right->vibe_request = 1;
      // Lock head position here?
      //Serial.println("Both Arms Stopped together");
    }
  }

  if (cpu_left->gesture_completed && cpu_right->gesture_completed) { // check for a head gesture?
    cpu_left->gesture_completed = false;
    cpu_right->gesture_completed = false;
    Serial.print("Gesture: ");
    print_gesture(cpu_left->gesture);
    Serial.print(" ");
    print_gesture(cpu_right->gesture);
    Serial.println(" ");

    if (cpu_left->gesture != 0 && cpu_right->gesture != 0) {
      cpu_left->vibe_request = 1;
      cpu_right->vibe_request = 1;
    }
  }

  vibe_update(cpu_left);
  vibe_update(cpu_right);


}
