#include <Arduino.h>
#include "gesture.h"

void print_cpu(struct cpu_struct *cpu_data) {

  Serial.print("\t");
  Serial.print(cpu_data->yaw);   //spinning, will need relative
  Serial.print("\t");

  if ( cpu_data->pitch > ( 128 + 30))
    Serial.print("U");
  else if ( cpu_data->pitch < (128 - 30))
    Serial.print("D");
  else
    Serial.print(cpu_data->pitch);
  Serial.print("\t");

  if ( cpu_data->roll > ( 128 + 30))
    Serial.print("CW");
  else if ( cpu_data->roll < (128 - 30))
    Serial.print("CCW");
  else
    Serial.print(cpu_data->roll);

}

void calc_rel_yaw(struct cpu_struct *cpu_data) {

  int angle_diff;

  if (((((int)cpu_data->yaw_slow) - cpu_data->yaw + 256) % 256) < 128) {
    angle_diff = cpu_data->yaw - cpu_data->yaw_slow;
    if (angle_diff < -128) {
      angle_diff = angle_diff + 256;
    }
    else if (angle_diff > 128) {
      angle_diff = angle_diff - 256;
    }
  }
  else {
    angle_diff = cpu_data->yaw_slow - cpu_data->yaw;

    if (angle_diff < -128) {
      angle_diff = angle_diff + 256;
    }
    else if (angle_diff > 128) {
      angle_diff = angle_diff - 256;
    }
    angle_diff = angle_diff * -1;
  }

  cpu_data->yaw_slow += .2 * angle_diff;

  if ( cpu_data->yaw_slow > 255)
    cpu_data->yaw_slow -= 255;

  if ( cpu_data->yaw_slow < 0)
    cpu_data->yaw_slow += 255;

  cpu_data->yaw_diff = angle_diff;
}


void gesture_check(struct cpu_struct *cpu_left, struct cpu_struct *cpu_right) {
  print_cpu(cpu_left);
  print_cpu(cpu_right);
  calc_rel_yaw(cpu_left);
  calc_rel_yaw(cpu_right);
  Serial.print("\t");
  Serial.println( cpu_left->yaw_diff);

}
