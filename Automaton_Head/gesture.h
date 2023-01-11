#ifndef _GESTURE_H
#define _GESTURE_H

#include "common.h"


#define GESTURE_CW   0b00000001
#define GESTURE_CCW  0b00000010
#define GESTURE_C    0b00111111
#define GESTURE_L    0b00000100
#define GESTURE_R    0b00001000
#define GESTURE_LR   0b00111100
#define GESTURE_U    0b00010000
#define GESTURE_D    0b00100000
#define GESTURE_UD   0b00110000
#define GESTURE_NO   0b11000000

 
#define ALLOWABLE_IDLE_TIME_VARIANCE 1000

#define GESTURE_TIMEOUT 2000 // max milliseconds a gesture can take


//gesture state machine 
#define GESTURE_IDLE            0
#define GESTURE_IDLE_COUNT      3     //how many idle ticks to wait until we start recognizing
#define GESTURE_RECOGNIZING     255 

// These set the required motion to detect a gesture
#define YAW_GESTURE 20
#define PITCH_GESTURE 20
#define ROLL_GESTURE 20

// These set the zone that idle is detected in
#define PITCH_DEADZONE (PITCH_GESTURE - 5)
#define ROLL_DEADZONE (ROLL_GESTURE -5)
#define YAW_DEADZONE (YAW_GESTURE - 5)


void gesture_check(struct cpu_struct *cpu_left,struct cpu_struct *cpu_right,struct cpu_struct * cpu_head);

#endif
