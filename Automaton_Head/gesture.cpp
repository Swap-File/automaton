#include <Arduino.h>
#include "gesture.h"
#include "vibe.h"

#define STOP_COUNT 5
#define STOP_WIGGLE_ROOM 1000

const int gesture_max_time = 2000;
static bool proccess_gesture = false;
//next steps
// function to record which all 6 motions happen during a movement (within timeout)
// determine end of momvement by moving back to center  (close enough)
// combine both hands of data, and determine priorty of moves, and output
//
int calc_relative_axis(uint8_t actual_angle, float * reference, float filter) {
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

void print_gesture(uint8_t gesture ) {
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

void calc_gesture(struct cpu_struct *cpu_data) {

  //priority up down, left right, then cc ccw
  int yaw_diff = calc_relative_axis(cpu_data->yaw, &(cpu_data->yaw_ref), 0.0);
  int pitch_diff = calc_relative_axis(cpu_data->pitch, &(cpu_data->pitch_ref), 0.0);
  int roll_diff = calc_relative_axis(cpu_data->roll, &(cpu_data->roll_ref), 0.0);

  if ( cpu_data->gesture_magnitude >  cpu_data->gesture_magnitude_last) {

    if ((cpu_data->gesture & GESTURE_C) == 0x00) {
      if ( roll_diff > 30)
        cpu_data->gesture = GESTURE_CW;
      if ( roll_diff  < -30)
        cpu_data->gesture = GESTURE_CCW;
    }

    if ((cpu_data->gesture & GESTURE_LR) == 0x00) {
      if ( yaw_diff > 20)
        cpu_data->gesture = GESTURE_L;
      if ( yaw_diff  < -20)
        cpu_data->gesture = GESTURE_R;
    }

    if ((cpu_data->gesture & GESTURE_UD) == 0x00) {
      if ( pitch_diff > 20)
        cpu_data->gesture = GESTURE_U;
      if ( pitch_diff  < -20)
        cpu_data->gesture = GESTURE_D;
    }
  } else {
    if ( cpu_data->gesture_handled == false && cpu_data->gesture != 0 ) {
      // print_gesture(cpu_data->gesture);

      if (millis() - cpu_data->centered_time < gesture_max_time && proccess_gesture) cpu_data->vibe_request = 1;

      cpu_data->gesture_handled = true;
    }
  }

  cpu_data->gesture_magnitude_last = cpu_data->gesture_magnitude;
}

void calc_relative(struct cpu_struct *cpu_data) {

  int yaw_diff = calc_relative_axis(cpu_data->yaw, &(cpu_data->yaw_filtered), 0.2);
  int pitch_diff = calc_relative_axis(cpu_data->pitch, &(cpu_data->pitch_filtered), 0.2);
  int roll_diff = calc_relative_axis(cpu_data->roll, &(cpu_data->roll_filtered), 0.2);

  cpu_data->gesture_magnitude = abs(roll_diff) +   abs(pitch_diff) + abs(yaw_diff);

  if ( abs(roll_diff) < 3 && abs(pitch_diff) < 3 && abs(yaw_diff) < 3 ) {  // this sets "stopped"
    if (cpu_data->centered_counter <= STOP_COUNT) {
      cpu_data->centered_counter++;
    }
  }

  if (cpu_data->centered_counter == STOP_COUNT ) {

    cpu_data->centered_time = millis();
    cpu_data->gesture_handled = false;
    cpu_data->gesture = 0;
    cpu_data->yaw_ref = cpu_data->yaw;
    cpu_data->pitch_ref = cpu_data->pitch;
    cpu_data->roll_ref = cpu_data->roll;
    Serial.print("hand Stopped ");
    Serial.println(cpu_data->centered_time);
  }

  if ( abs(roll_diff) > 10 || abs(pitch_diff) > 10 || abs(yaw_diff) > 10 ) {  //this sets how far we need to move to break out of the stopped point
    cpu_data->centered_counter = 0;
  }
}

void gesture_check(struct cpu_struct *cpu_left, struct cpu_struct *cpu_right, struct cpu_struct* cpu_head) {

  static bool stopped = false;
  

  calc_relative(cpu_left);
  calc_relative(cpu_right);
  calc_relative(cpu_head);

  calc_gesture(cpu_left);
  calc_gesture(cpu_right);

  if (abs((int64_t)cpu_left->centered_time - (int64_t)cpu_right->centered_time) < STOP_WIGGLE_ROOM) {
    if (stopped == false) {
      Serial.println("Both Arms Stopped together");
      stopped = true;
      cpu_left->vibe_request = 1;
      cpu_right->vibe_request = 1;
      proccess_gesture = true;
    }
  } else {
    stopped = false;
  }


  if  (cpu_left->gesture_handled == true && cpu_right->gesture_handled == true && proccess_gesture) {

    if (millis() - cpu_left->centered_time < gesture_max_time && millis() - cpu_right->centered_time < gesture_max_time) {
      Serial.print("Good: ");
    } else {
      Serial.print("Late: ");
    }

    Serial.print("Gesture: ");
    print_gesture(cpu_left->gesture);
    Serial.print(" ");
    print_gesture(cpu_right->gesture);
    Serial.println(" ");
    proccess_gesture = false;

  }
  vibe_update(cpu_left);
  vibe_update(cpu_right);
  //calc_gesture(cpu_head);

}
