#include <Arduino.h>
#include "gesture.h"




int calc_rel_axis(uint8_t actual_angle, float * reference, float filter) {

  int angle_diff;

  if (((((int)*reference) - actual_angle + 256) % 256) < 128) {
    angle_diff = actual_angle - *reference;
    if (angle_diff < -128) {
      angle_diff = angle_diff + 256;
    }
    else if (angle_diff > 128) {
      angle_diff = angle_diff - 256;
    }
  }
  else {
    angle_diff = *reference - actual_angle;

    if (angle_diff < -128) {
      angle_diff = angle_diff + 256;
    }
    else if (angle_diff > 128) {
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


void print_cpu(struct cpu_struct *cpu_data) {


  int yaw_diff = calc_rel_axis(cpu_data->yaw, &(cpu_data->yaw_ref), 0.0);
  int pitch_diff = calc_rel_axis(cpu_data->pitch, &(cpu_data->pitch_ref), 0.0);
  int roll_diff = calc_rel_axis(cpu_data->roll, &(cpu_data->roll_ref), 0.0);

  Serial.print("\t");

  if ( yaw_diff > 20)
    Serial.print("L");
  else if ( yaw_diff  < -20)
    Serial.print("R");
  else
    Serial.print( yaw_diff);   //spinning, will need relative
  Serial.print("\t");

  if (pitch_diff > 30)
    Serial.print("U");
  else if ( pitch_diff < -30)
    Serial.print("D");
  else
    Serial.print(pitch_diff);
  Serial.print("\t");

  if ( roll_diff > 30)
    Serial.print("CW");
  else if ( roll_diff < -30)
    Serial.print("CCW");
  else
    Serial.print(roll_diff);

}


void calc_rel(struct cpu_struct *cpu_data) {
  
  int yaw_diff = calc_rel_axis(cpu_data->yaw, &(cpu_data->yaw_filtered), 0.2);
  int pitch_diff = calc_rel_axis(cpu_data->pitch, &(cpu_data->pitch_filtered), 0.2);
  int roll_diff = calc_rel_axis(cpu_data->roll, &(cpu_data->roll_filtered), 0.2);


  if ( abs(roll_diff) < 2 && abs(pitch_diff) < 2 && abs(yaw_diff) < 2 ) {
    if (cpu_data->recenter_counter < 10)
      cpu_data->recenter_counter++;
  } else {
    cpu_data->recenter_counter = 0;
  }

  if (cpu_data->recenter_counter == 5) {
    cpu_data->recenter_counter++;
    Serial.println("stopped");
    cpu_data->yaw_ref = cpu_data->yaw;
    cpu_data->pitch_ref = cpu_data->pitch;
    cpu_data->roll_ref = cpu_data->roll;
  }
}

void gesture_check(struct cpu_struct *cpu_left, struct cpu_struct *cpu_right, struct cpu_struct* cpu_head) {


  calc_rel(cpu_left);
  // calc_rel(cpu_right);
  // calc_rel(cpu_head);

  print_cpu(cpu_left);
  print_cpu(cpu_right);
  //print_cpu(cpu_head);
  Serial.print("\t");
  Serial.println();

}
