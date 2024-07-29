#include <Arduino.h>
#include "FastCRC.h"
#include "cobs.h"
#include "fin.h"
#include "mem.h"
#include "common.h"
static FastCRC8 CRC8;

#define SERVO_ON_TIME 1000
static struct fin_struct fin_array[FIN_NUM];

#define FIN_HANDLED 255
static uint8_t servo_mode = FIN_UP;


uint8_t fin_pos_target[15];
uint8_t fin_pos1[15] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };  //up
uint8_t fin_pos2[15] = { 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127 };
uint8_t fin_pos3[15] = { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 };  //down


uint32_t fin_execute_time[15];


uint32_t fin_immediate[15] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
uint32_t fin_left_up[15] = { 0, 100, 100, 200, 200, 300, 300, 400, 400, 500, 500, 600, 600, 700, 700 };
uint32_t fin_right_up[15] = { 700, 700, 600, 600, 500, 500, 400, 400, 300, 300, 200, 200, 100, 100, 0 };
uint32_t fin_left_down[15] = { 0, 0, 100, 100, 200, 200, 300, 300, 400, 400, 500, 500, 600, 600, 700 };
uint32_t fin_right_down[15] = { 700, 600, 600, 500, 500, 400, 400, 300, 300, 200, 200, 100, 100, 0, 0 };
uint32_t fin_alt_up[15] = { 0, 200, 100, 300, 0, 200, 100, 300, 100, 200, 0, 300, 100, 200, 0 };
uint32_t fin_alt_down[15] = { 300, 100, 200, 0, 300, 100, 200, 0, 200, 100, 300, 0, 200, 100, 300 };

static uint8_t fin_effect = FIN_IMMEDIATE;
static uint8_t servo_mode_last = 255;

void fin_set(int mode, int effect) {
  servo_mode = mode;
  fin_effect = effect;
}

void fin_set(int mode) {
  servo_mode = mode;
}

int fin_random(void) {
  int last_mode = servo_mode;
  int last_effect = fin_effect;

  while (last_mode == servo_mode) {
    servo_mode = random(FIN_MODE_FIRST, FIN_MODE_LAST);
  }
  while (last_effect == fin_effect) {
    fin_effect = random(FIN_EFFECT_FIRST, FIN_EFFECT_LAST);
  }
  return servo_mode;
}

int fin_mode(void) {
  return servo_mode_last;
}

#define FIN_BUMP_SPEED 50

static int fin_bump_offset = 0;
static int fin_bump_direction = 0;
static int fin_bump_index = 0;
static uint32_t fin_bump_time = 0;

void fin_bump(int dir, int distance) {
  fin_bump_direction = dir;
  fin_bump_offset = 0;

  if (dir > 0)
    fin_bump_index = 0;
  else
    fin_bump_index = 14;

  fin_bump_time = millis() + FIN_BUMP_SPEED;
}

int test_left = 0;
int test_left_counter = 0;

void fin_smooth_left(int dir) {
  test_left = dir;
}

int test_right = 0;
int test_right_counter = 0;

void fin_smooth_right(int dir) {
  test_right = dir;
}


void set_the_fin(int i, int val) {
  fin_array[i].servo = constrain(map(val, 0, 255, mem_get_min(i), mem_get_max(i)), mem_get_min(i), mem_get_max(i));
}

void idle_fins() {

  //output
  for (int i = 0; i < FIN_NUM; i++) {
    if (fin_array[i].servo != fin_array[i].servo_last) {
      fin_array[i].servo_last = fin_array[i].servo;
      fin_array[i].servo_time = millis() + SERVO_ON_TIME;
    }
    if (millis() > fin_array[i].servo_time && millis() > SERVO_ON_TIME * 4)
      fin_array[i].servo_sent = 0;
    else
      fin_array[i].servo_sent = fin_array[i].servo;
  }
}

static bool check_if_animation_done(void) {
  for (int i = 0; i < FIN_NUM; i++) {
    if (fin_execute_time[i] > millis()) return false;
  }
  return true;
}

static void map_servos_animation(uint8_t servos[]) {

  if (servo_mode != FIN_HANDLED) {

    // handle the same request twice in a row by immediate movement
    if (servo_mode == servo_mode_last) {
      for (int i = 0; i < FIN_NUM; i++) {
        fin_array[i].servo_time = millis() + SERVO_ON_TIME;
        fin_execute_time[i] = millis();
      }
      servo_mode = FIN_HANDLED;
    }

    //handle new requests and select proper transitions to prevent fin collision
    if (servo_mode == FIN_DOWN) {

      memcpy(fin_pos_target, fin_pos3, 15);
      if (fin_effect == FIN_IMMEDIATE) {
        for (int i = 0; i < FIN_NUM; i++) {
          fin_execute_time[i] = millis() + fin_immediate[i];
        }
      }
      if (fin_effect == FIN_LEFT) {
        for (int i = 0; i < FIN_NUM; i++) {
          fin_execute_time[i] = millis() + fin_left_down[i];
        }
      }
      if (fin_effect == FIN_RIGHT) {

        for (int i = 0; i < FIN_NUM; i++) {
          fin_execute_time[i] = millis() + fin_right_down[i];
        }
      }
      if (fin_effect == FIN_ALT) {
        for (int i = 0; i < FIN_NUM; i++) {
          fin_execute_time[i] = millis() + fin_alt_down[i];
        }
      }
    }
    if (servo_mode == FIN_MID) {

      memcpy(fin_pos_target, fin_pos2, 15);

      if (servo_mode_last == FIN_UP) {
        if (fin_effect == FIN_IMMEDIATE) {
          for (int i = 0; i < FIN_NUM; i++) {
            fin_execute_time[i] = millis() + fin_immediate[i];
          }
        }
        if (fin_effect == FIN_LEFT) {
          for (int i = 0; i < FIN_NUM; i++) {
            fin_execute_time[i] = millis() + fin_left_down[i];
          }
        }
        if (fin_effect == FIN_RIGHT) {

          for (int i = 0; i < FIN_NUM; i++) {
            fin_execute_time[i] = millis() + fin_right_down[i];
          }
        }
        if (fin_effect == FIN_ALT) {
          for (int i = 0; i < FIN_NUM; i++) {
            fin_execute_time[i] = millis() + fin_alt_down[i];
          }
        }
      }

      if (servo_mode_last == FIN_DOWN) {
        if (fin_effect == FIN_IMMEDIATE) {
          for (int i = 0; i < FIN_NUM; i++) {
            fin_execute_time[i] = millis() + fin_immediate[i];
          }
        }
        if (fin_effect == FIN_LEFT) {
          for (int i = 0; i < FIN_NUM; i++) {
            fin_execute_time[i] = millis() + fin_left_up[i];
          }
        }
        if (fin_effect == FIN_RIGHT) {

          for (int i = 0; i < FIN_NUM; i++) {
            fin_execute_time[i] = millis() + fin_right_up[i];
          }
        }
        if (fin_effect == FIN_ALT) {
          for (int i = 0; i < FIN_NUM; i++) {
            fin_execute_time[i] = millis() + fin_alt_up[i];
          }
        }
      }
    }

    if (servo_mode == FIN_UP) {
      memcpy(fin_pos_target, fin_pos1, 15);
      if (fin_effect == FIN_IMMEDIATE) {
        for (int i = 0; i < FIN_NUM; i++) {
          fin_execute_time[i] = millis() + fin_immediate[i];
        }
      }
      if (fin_effect == FIN_LEFT) {
        for (int i = 0; i < FIN_NUM; i++) {
          fin_execute_time[i] = millis() + fin_left_up[i];
        }
      }
      if (fin_effect == FIN_RIGHT) {

        for (int i = 0; i < FIN_NUM; i++) {
          fin_execute_time[i] = millis() + fin_right_up[i];
        }
      }
      if (fin_effect == FIN_ALT) {
        for (int i = 0; i < FIN_NUM; i++) {
          fin_execute_time[i] = millis() + fin_alt_up[i];
        }
      }
    }

    servo_mode_last = servo_mode;
    servo_mode = FIN_HANDLED;
  }

  //apply position / animation
  for (int i = 0; i < FIN_NUM; i++) {
    if (millis() > fin_execute_time[i]) {
      set_the_fin(i, fin_pos_target[i]);  //check later
    }
  }

  if (fin_bump_direction != 0 && fin_bump_time < millis()) {
    fin_bump_index += fin_bump_direction;
    fin_bump_time = millis() + FIN_BUMP_SPEED;
    if (fin_bump_index > 14 || fin_bump_index < 0)
      fin_bump_direction = 0;
  }

  //apply bump
  if (fin_bump_direction != 0) {
    set_the_fin(fin_bump_index, fin_array[fin_bump_index].servo + fin_bump_offset);  //check later
  }

  idle_fins();
}


static void map_x_leds(const CRGB* x_leds) {
  int idx = 0;
  for (int i = 0; i < FIN_NUM; i++) {
    fin_array[i].strip[0] += x_leds[idx++];
    fin_array[i].led += x_leds[idx];  //middle led maps to the same as this
    fin_array[i].strip[1] += x_leds[idx++];
    fin_array[i].strip[2] += x_leds[idx++];
  }
}

static void map_y_leds(const CRGB* y_leds) {
  int row_one_front = 3;
  int row_one_back = 4;
  int row_two_front = 5;
  int row_two_back = 6;

  int idx = 0;
  for (int i = 0; i < FIN_NUM; i++) {
    if (i % 2) {
      fin_array[i].strip[0] += y_leds[row_two_front];
      fin_array[i].led += y_leds[row_two_back];
      fin_array[i].strip[1] += y_leds[row_two_front];
      fin_array[i].strip[2] += y_leds[row_two_front];
    } else {
      fin_array[i].strip[0] += y_leds[row_one_front];
      fin_array[i].led += y_leds[row_one_back];
      fin_array[i].strip[1] += y_leds[row_one_front];
      fin_array[i].strip[2] += y_leds[row_one_front];
    }
  }
}

void clear_leds(void) {
  for (int i = 0; i < FIN_NUM; i++) {
    fin_array[i].strip[0] = CRGB(0, 0, 0);
    fin_array[i].led = CRGB(0, 0, 0);
    fin_array[i].strip[1] = CRGB(0, 0, 0);
    fin_array[i].strip[2] = CRGB(0, 0, 0);
  }
}

#define NUM_FRONT_FINS 8
#define NUM_BACK_FINS 7

void map_servos_smooth(int left_roll, int right_roll, int z, int w) {

  //center all the servos
  int front_fins[NUM_FRONT_FINS] = { 127, 127, 127, 127, 127, 127, 127, 127 };
  int back_fins[NUM_BACK_FINS] = { 255, 255, 255, 255, 255, 255, 255 };

  int slope = -64;
  int fins_allowed = 4;  //set high

  int max_roll = -1 * slope * (fins_allowed + 1);



  //calculate left hand servo pull
  if (left_roll >= 0) {
    left_roll = min(left_roll, max_roll);
    for (int i = 0; i < NUM_FRONT_FINS; i++) {
      int offset = max(slope * i + left_roll, 0);
      front_fins[i] -= max(offset, 0);
    }

  } else {
    left_roll *= -1;
    left_roll = min(left_roll, max_roll);
    for (int i = 0; i < NUM_FRONT_FINS; i++) {
      int offset = max(slope * i + left_roll, 0);
      front_fins[i] += min(offset, 255);
    }
  }

  //calculate right hand servo pull
  if (right_roll >= 0) {
    right_roll = min(right_roll, max_roll);
    for (int i = 0; i < NUM_FRONT_FINS; i++) {
      int offset = max(slope * i + right_roll, 0);
      front_fins[NUM_FRONT_FINS - i] -= max(offset, 0);
    }

  } else {
    right_roll *= -1;
    right_roll = min(right_roll, max_roll);
    for (int i = 0; i < NUM_FRONT_FINS; i++) {
      int offset = max(slope * i + right_roll, 0);
      front_fins[NUM_FRONT_FINS - i] += min(offset, 255);
    }
  }



  int front_fins_untouched[NUM_FRONT_FINS];
  int front_fins_exaggerated[NUM_FRONT_FINS];

  for (int i = 0; i < 4; i++) {
    front_fins_untouched[i] = front_fins[i];
    front_fins[i] = constrain(constrain(front_fins[i], 0, 255) + w / 2, 0, 255);  //give half the motion to front fins
    front_fins_exaggerated[i] = constrain(constrain(front_fins[i], 0, 255) + w, 0, 255);
  }
  for (int i = 4; i < 8; i++) {
    front_fins_untouched[i] = front_fins[i];
    front_fins[i] = constrain(constrain(front_fins[i], 0, 255) + z / 2, 0, 255);
    front_fins_exaggerated[i] = constrain(constrain(front_fins[i], 0, 255) + w, 0, 255);
  }




  //set the front fins
  int fin_counter = 0;
  for (int i = 0; i < (NUM_FRONT_FINS + NUM_BACK_FINS); i++) {
    if (i % 2 == 0) {
      front_fins[fin_counter] = constrain(front_fins[fin_counter], 0, 255);
      set_the_fin(i, front_fins[fin_counter++]);
    }
  }




  int back_fins_limit[NUM_BACK_FINS];
  fin_counter = 0;
  //process back fins here
  for (int i = 0; i < NUM_BACK_FINS; i++) {

    back_fins_limit[i] = max(front_fins[fin_counter], front_fins[fin_counter + 1]);
    back_fins_limit[i] = 255 - (2 * (255 - back_fins_limit[i]));  //back fin limit could be 2x, 3x, or squared

    back_fins[i] = max(front_fins_exaggerated[fin_counter], front_fins_exaggerated[fin_counter + 1]);
    back_fins[i] = min(back_fins[i], back_fins_limit[i]);

    fin_counter++;
  }

  //set back fins here
  fin_counter = 0;
  for (int i = 0; i < (NUM_FRONT_FINS + NUM_BACK_FINS); i++) {
    if (i % 2 == 1) {
      back_fins[fin_counter] = constrain(back_fins[fin_counter], 0, 255);
      set_the_fin(i, back_fins[fin_counter++]);
    }
  }
}


void fin_update(struct led_struct* led_data) {

  static bool transition = true;

  if (led_data->fin_effect == 0 || (check_if_animation_done() == false)) {
    map_servos_animation(led_data->servos);

  } else if (led_data->fin_effect == 1) {
    map_servos_smooth(led_data->left_roll, led_data->right_roll, led_data->right_pitch, led_data->left_pitch);
  }

  idle_fins();

  clear_leds();

  if (led_data->off == false) {
    if (led_data->sound_fins) {
      map_x_leds(led_data->x_fft_leds);
    } else {
      map_x_leds(led_data->x_leds);
      map_y_leds(led_data->y_leds);
    }
  }

  uint8_t raw_buffer[195 + 1];  //data + CRC

  int idx = 0;
  for (int i = 0; i < FIN_NUM; i++) {

    raw_buffer[idx++] = fin_array[i].strip[0].r;
    raw_buffer[idx++] = fin_array[i].strip[0].g;
    raw_buffer[idx++] = fin_array[i].strip[0].b;
    raw_buffer[idx++] = fin_array[i].strip[1].r;
    raw_buffer[idx++] = fin_array[i].strip[1].g;
    raw_buffer[idx++] = fin_array[i].strip[1].b;
    raw_buffer[idx++] = fin_array[i].strip[2].r;
    raw_buffer[idx++] = fin_array[i].strip[2].g;
    raw_buffer[idx++] = fin_array[i].strip[2].b;
    raw_buffer[idx++] = fin_array[i].led.r;
    raw_buffer[idx++] = fin_array[i].led.g;
    raw_buffer[idx++] = fin_array[i].led.b;
    raw_buffer[idx++] = fin_array[i].servo_sent;
  }
  //idx = 195  sizeof(fin_array) = 195

  raw_buffer[195] = CRC8.maxim(raw_buffer, 195);


  uint8_t encoded_buffer[195 + 3];  // data + 1 CRC + 2 potential cobs bytes
  uint8_t encoded_size = COBSencode(raw_buffer, 195 + 1, encoded_buffer);
  Serial1.write(encoded_buffer, encoded_size);
  Serial1.write((uint8_t)0x00);
}
