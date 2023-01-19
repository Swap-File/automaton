#include <Arduino.h>
#include <OneWire.h>
#include <cobs.h>

uint8_t wing_data[60] = { 0 };


void wing_update(void) {

  
  uint8_t raw_buffer[15];

  raw_buffer[14] = OneWire::crc8(raw_buffer, 14);
  uint8_t encoded_buffer[16];
  uint8_t encoded_size = COBSencode(raw_buffer, 15, encoded_buffer);
  Serial2.write(encoded_buffer, encoded_size);


  Serial2.write(0x00);
  Serial1.write(wing_data, 60);  // 7 to 10ms at 115200 baud    2ms , worst 5ms  at 500000 baud


}
