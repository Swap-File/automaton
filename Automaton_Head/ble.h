#ifndef _BLR_H
#define _BLR_H

#include "common.h"

void ble_init(struct cpu_struct *cpu_left,struct cpu_struct *cpu_right);
void ble_notify(bool left, bool right);

#endif
