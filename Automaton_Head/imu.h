#ifndef _IMU_H
#define _IMU_H

#include "common.h"

void imu_init(void);
void imu_update(struct cpu_struct *hand_data);

#endif
