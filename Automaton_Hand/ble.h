#ifndef _BLR_H
#define _BLR_H

#include "common.h"

void ble_init(bool hand);
bool ble_send(struct cpu_struct *hand_data);

#endif
