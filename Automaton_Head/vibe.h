#ifndef _VIBE_H
#define _VIBE_H

#define VIBE_LEFT 0
#define VIBE_RIGHT 1

#include "common.h"

void vibe_update(struct cpu_struct *hand_data);
uint8_t vibe_read(bool  left, bool right);

#endif
