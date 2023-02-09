#include <Arduino.h>
#include "FastCRC.h"
#include "cobs.h"
#include "fin.h"
#include "common.h"
static FastCRC8 CRC8;


static struct fin_struct fin_array[FIN_NUM];

static void map_servos(uint8_t servos[]) {
  for (int i = 0; i < FIN_NUM; i++) {
    if ( servos[i] == 255) {
      fin_array[i].servo = servo_max[i];
    } else if ( servos[i] == 0) {
      fin_array[i].servo = servo_min[i];
    } else {
      fin_array[i].servo = map( servos[i], 0, 255, servo_min[i], servo_max[i]);
      fin_array[i].servo = constrain(fin_array[i].servo, servo_min[i], servo_max[i]);
    }
  }
}

static void map_x_leds(const CRGB * x_leds) {
  int idx = 0;
  for (int i = 0; i < FIN_NUM; i++) {
    fin_array[i].strip[0] = x_leds[idx++];
    fin_array[i].led = x_leds[idx]; //middle led maps to the same as this
    fin_array[i].strip[1] = x_leds[idx++];
    fin_array[i].strip[2] = x_leds[idx++];
  }
}

void fin_update(struct led_struct * led_data) {

  map_servos(led_data->servos);
  map_x_leds(led_data->x_leds);

  uint8_t raw_buffer[sizeof(fin_array) + 1]; //data + CRC

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
    raw_buffer[idx++] = fin_array[i].servo;
  }
  //idx = 195  sizeof(fin_array) = 195

  raw_buffer[sizeof(fin_array)] = CRC8.maxim(raw_buffer, sizeof(fin_array));


  uint8_t encoded_buffer[sizeof(fin_array) + 3]; // data + 1 CRC + 2 potential cobs bytes
  uint8_t encoded_size = COBSencode(raw_buffer, sizeof(fin_array) + 1, encoded_buffer);
  Serial1.write(encoded_buffer, encoded_size);
  Serial1.write((uint8_t)0x00);

}
