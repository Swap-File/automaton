#include <Arduino.h>
#include "FastCRC.h"
#include "cobs.h"
#include "fin.h"
#include "mem.h"
#include "common.h"
static FastCRC8 CRC8;

#define SERVO_ON_TIME 250
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


static void map_servos(uint8_t servos[]) {




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
      fin_array[i].servo = constrain(map(fin_pos_target[i], 0, 255, mem_get_min(i), mem_get_max(i)), mem_get_min(i), mem_get_max(i));
    }
  }

  if (fin_bump_direction != 0 && fin_bump_time < millis()) {
  //  Serial.print(fin_bump_index);
   // Serial.println(" Bump");
    fin_bump_index += fin_bump_direction;
    fin_bump_time = millis() + FIN_BUMP_SPEED;
    if (fin_bump_index > 14 || fin_bump_index < 0)
      fin_bump_direction = 0;
  }

  //apply bump
  if (fin_bump_direction != 0) {
    fin_array[fin_bump_index].servo = constrain(map(fin_array[fin_bump_index].servo + fin_bump_offset, 0, 255, mem_get_min(fin_bump_index), mem_get_max(fin_bump_index)), mem_get_min(fin_bump_index), mem_get_max(fin_bump_index));
  }



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

void fin_update(struct led_struct* led_data) {

  map_servos(led_data->servos);

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
