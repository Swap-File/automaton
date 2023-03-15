#include <Arduino.h>
#include "FastCRC.h"
#include "cobs.h"
#include "fin.h"
#include "common.h"
static FastCRC8 CRC8;


static struct fin_struct fin_array[FIN_NUM];


//allow per servo settings
uint8_t servo_min[15];
uint8_t servo_max[15];
uint8_t servo_mode = 0;  // 0 is auto 1 is up 2 is down


static void map_servos(uint8_t servos[]) {
  if (servo_mode == 0) {
    for (int i = 0; i < FIN_NUM; i++) {
      if (servos[i] == 255) {
        fin_array[i].servo = servo_max[i];
      } else if (servos[i] == 0) {
        fin_array[i].servo = servo_min[i];
      } else {
        fin_array[i].servo = constrain(map(servos[i], 0, 255, servo_min[i], servo_max[i]), servo_min[i], servo_max[i]);
      }
    }
  }

  if (servo_mode == 1) {
    for (int i = 0; i < FIN_NUM; i++)
      fin_array[i].servo = servo_max[i];
  }
  if (servo_mode == 2) {
    for (int i = 0; i < FIN_NUM; i++)
      fin_array[i].servo = servo_min[i];
  }
  for (int i = 0; i < FIN_NUM; i++) {
    if (fin_array[i].servo != fin_array[i].servo_last) {
      fin_array[i].servo_last = fin_array[i].servo;
      fin_array[i].servo_time = millis() + 250;
    }
    if (millis() > fin_array[i].servo_time)
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
  map_x_leds(led_data->x_leds);
  map_y_leds(led_data->y_leds);

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
