#ifndef _VIBE_H
#define _VIBE_H

#define VIBE_LEFT 0
#define VIBE_RIGHT 1

#include "common.h"

void vibe_update(bool * left, bool *right);
uint8_t vibe_read(bool  left, bool right);
void vibe_add_quick_slow(bool side, int quick, int slow);
void vibe_add_slow_quick(bool side, int slow, int quick);

#endif
