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


static int calc_relative_axis_angle(uint8_t actual_angle, float *reference, float filter) {
  int angle_diff = calc_relative_axis_angle(actual_angle, *reference);
  *reference += filter * angle_diff;
  if (*reference > 255)
    *reference -= 255;
  if (*reference < 0)
    *reference += 255;
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

static void check_for_gestures(struct cpu_struct *cpu_data) {

  //priority of gestures:  up down, left right, then cc ccw

  int yaw_diff = calc_relative_axis_angle(cpu_data->yaw, cpu_data->yaw_ref);
  int pitch_diff = calc_relative_axis_angle(cpu_data->pitch, cpu_data->pitch_ref);
  int roll_diff = calc_relative_axis_angle(cpu_data->roll, cpu_data->roll_ref);

  if ((cpu_data->gesture & GESTURE_C) == 0x00) {
    if (roll_diff > ROLL_GESTURE) {
      cpu_data->gesture = GESTURE_CW;
    }
    if (roll_diff < -ROLL_GESTURE) {
      cpu_data->gesture = GESTURE_CCW;
    }
  }

  if ((cpu_data->gesture & GESTURE_LR) == 0x00) {
    if (yaw_diff > YAW_GESTURE) {
      cpu_data->require_new_yaw = true;
      cpu_data->gesture = GESTURE_L;
    }
    if (yaw_diff < -YAW_GESTURE) {
      cpu_data->require_new_yaw = true;
      cpu_data->gesture = GESTURE_R;
    }
  }

  if ((cpu_data->gesture & GESTURE_UD) == 0x00) {
    if (pitch_diff > PITCH_GESTURE) {
      cpu_data->gesture = GESTURE_U;
    }
    if (pitch_diff < -PITCH_GESTURE) {
      cpu_data->gesture = GESTURE_D;
    }
  }
}

static void set_reference_angles(struct cpu_struct *cpu_data) {
  cpu_data->gesture_start_time = millis();
  cpu_data->gesture = 0;
  cpu_data->yaw_ref = cpu_data->yaw;
  cpu_data->pitch_ref = 128;  // fixed start angle of straight ahead
  cpu_data->roll_ref = 128;   // fixed start angle of level roll
  //cpu_data->pitch_ref = cpu_data->pitch; // allow dynamic start angles
  //cpu_data->roll_ref = cpu_data->roll;   // allow dynamic start angles

  //Serial.print(cpu_data->id);
  //Serial.print(" stopped at ");
  //Serial.println(cpu_data->gesture_start_time);
}

static inline bool check_absolute_deadzone(uint8_t angle, int deadzone) {
  if (angle > 128 - deadzone && angle < 128 + deadzone)
    return true;
  return false;
}

static inline bool check_relative_deadzone(uint8_t angle, float angle_last_gesture, int deadzone) {
  int angle_diff = calc_relative_axis_angle(angle, angle_last_gesture);
  if (abs(angle_diff) > deadzone)
    return true;
  return false;
}

bool check_deadzone(struct cpu_struct *cpu_data) {
  if (!check_absolute_deadzone(cpu_data->pitch, PITCH_DEADZONE)) {
    cpu_data->require_new_yaw = false;

    return false;
  }
  if (!check_absolute_deadzone(cpu_data->roll, ROLL_DEADZONE)) {
    cpu_data->require_new_yaw = false;

    return false;
  }


  if (cpu_data->require_new_yaw) {
    if (!check_relative_deadzone(cpu_data->yaw, cpu_data->yaw_last_gesture, YAW_DEADZONE))
      return false;
  }
  return true;
}

bool check_gesture(struct cpu_struct *cpu_data) {


  int yaw_diff = calc_relative_axis_angle(cpu_data->yaw, cpu_data->yaw_ref);
  int pitch_diff = calc_relative_axis_angle(cpu_data->pitch, cpu_data->pitch_ref);
  int roll_diff = calc_relative_axis_angle(cpu_data->roll, cpu_data->roll_ref);


  if (cpu_data->id == 'H') {  //change limits for the head

    if (yaw_diff > YAW_GESTURE)
      return true;
    if (yaw_diff < -YAW_GESTURE)
      return true;

  } else {
    if (roll_diff > ROLL_GESTURE)
      return true;
    if (roll_diff < -ROLL_GESTURE)
      return true;

    if (yaw_diff > YAW_GESTURE)
      return true;
    if (yaw_diff < -YAW_GESTURE)
      return true;

    if (pitch_diff > PITCH_GESTURE)
      return true;
    if (pitch_diff < -PITCH_GESTURE)
      return true;
  }

  return false;
}

void complete_gesture(struct cpu_struct *cpu_data) {
  cpu_data->yaw_last_gesture = cpu_data->yaw;

  cpu_data->gesture_state = GESTURE_IDLE;
  cpu_data->gesture_complete_flag = true;
  cpu_data->gesture_start_flag = false;
}

static void compute_gestures(struct cpu_struct *cpu_data) {

  int yaw_diff = calc_relative_axis_angle(cpu_data->yaw, &(cpu_data->yaw_filtered), 0.5);
  int pitch_diff = calc_relative_axis_angle(cpu_data->pitch, &(cpu_data->pitch_filtered), 0.5);
  int roll_diff = calc_relative_axis_angle(cpu_data->roll, &(cpu_data->roll_filtered), 0.5);

  if (cpu_data->gesture_state < GESTURE_IDLE_COUNT) {
    if (abs(roll_diff) < 3 && abs(pitch_diff) < 3 && abs(yaw_diff) < 3 && check_deadzone(cpu_data)) {

      cpu_data->gesture_complete_flag = false;
      cpu_data->gesture_state++;
      if (cpu_data->gesture_state == GESTURE_IDLE_COUNT) {
        set_reference_angles(cpu_data);

        cpu_data->gesture_start_flag = true;
        cpu_data->gesture_start_time = millis();
        cpu_data->gesture_state = GESTURE_RECOGNIZING;
      }
    }
  }
  /*
    Serial.print( yaw_diff);
    Serial.print( '\t');
    Serial.print( pitch_diff);
    Serial.print( '\t');
    Serial.println( roll_diff);
  */

  if (cpu_data->gesture_state >= GESTURE_RECOGNIZING) {
    if (abs(roll_diff) < 5 && abs(pitch_diff) < 5 && abs(yaw_diff) < 5 && check_gesture(cpu_data)) {
      cpu_data->gesture_state++;
      if (cpu_data->gesture_state == GESTURE_RECOGNIZED) {
        check_for_gestures(cpu_data);
        cpu_data->gesture_complete_good_flag = true;
        complete_gesture(cpu_data);
      }
    }
  }


  if (cpu_data->gesture_state >= GESTURE_RECOGNIZING) {
    if (millis() - cpu_data->gesture_start_time > GESTURE_TIMEOUT) {
      complete_gesture(cpu_data);
      cpu_data->require_new_yaw = true;
    }
  }
}

inline bool check_head(const struct cpu_struct *cpu_data) {
  if (cpu_data->roll > 128 + ROLL_DEADZONE)  //or ROLL_DEADZONE
    return false;
  if (cpu_data->roll < 128 - ROLL_DEADZONE)
    return false;



  if (cpu_data->pitch > 128 + PITCH_DEADZONE)  //or PITCH_DEADZONE
    return false;
  if (cpu_data->pitch < 128 - PITCH_DEADZONE)
    return false;

  return true;
}

void gesture_check(struct cpu_struct *cpu_left, struct cpu_struct *cpu_right, struct cpu_struct *cpu_head) {
  compute_gestures(cpu_left);
  compute_gestures(cpu_right);

  static bool in_progress = false;
  if (in_progress)
    compute_gestures(cpu_head);

  if (cpu_left->gesture_start_flag && cpu_right->gesture_start_flag) {
    if ((abs((int64_t)cpu_left->gesture_start_time - (int64_t)cpu_right->gesture_start_time) < ALLOWABLE_IDLE_TIME_VARIANCE) && check_head(cpu_head)) {
      cpu_left->gesture_start_flag = false;
      cpu_right->gesture_start_flag = false;

      cpu_left->vibe_request = 1;
      cpu_right->vibe_request = 1;
      // Lock head position here?

      set_reference_angles(cpu_head);
      cpu_head->gesture_state = GESTURE_RECOGNIZING;
      cpu_head->gesture_start_flag = true;
      cpu_head->gesture_start_time = millis();
      in_progress = true;
      //Serial.println("Both Arms Stopped together");
    }
  }

  if ((cpu_left->gesture_complete_good_flag && cpu_right->gesture_complete_good_flag) || cpu_head->gesture_complete_good_flag) {  // check for a head gesture?
    if (cpu_head->gesture_complete_flag) {
      cpu_left->require_new_yaw = true;
      cpu_right->require_new_yaw = true;
    }
    if (cpu_head->gesture_complete_good_flag) {
      Serial.print("Head movement: \t");
      print_gesture(cpu_head->gesture);
      Serial.println(" ");
      cpu_left->vibe_request = 1;
      cpu_right->vibe_request = 1;
    }
    in_progress = false;
    cpu_left->gesture_complete_flag = false;
    cpu_right->gesture_complete_flag = false;
    cpu_head->gesture_complete_flag = false;
    cpu_left->gesture_complete_good_flag = false;
    cpu_right->gesture_complete_good_flag = false;
    cpu_head->gesture_complete_good_flag = false;
    cpu_head->gesture_state = GESTURE_IDLE;

    if (cpu_left->gesture != 0 && cpu_right->gesture != 0) {
      cpu_left->vibe_request = 1;
      cpu_right->vibe_request = 1;
      Serial.print("Hand movement: \t");
      print_gesture(cpu_left->gesture);
      Serial.print("\t");
      print_gesture(cpu_right->gesture);
      Serial.println("\t");
    }
  }




  vibe_update(cpu_left);
  vibe_update(cpu_right);
}
