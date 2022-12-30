#ifndef _VIBE_H
#define _VIBE_H

void vibe_init(void);
void vibe_update(void);
bool vibe_active(void);

void vibe_add_quick_slow(int quick, int slow);
void vibe_add_slow_quick(int slow, int quick);


#endif
