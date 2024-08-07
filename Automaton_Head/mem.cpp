#include <Arduino.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include "common.h"
#include "fin.h"

using namespace Adafruit_LittleFS_Namespace;

File file(InternalFS);

//allow per servo settings
uint8_t servo_min[15];
uint8_t servo_max[15];

int mem_get_min(int i) {
  return servo_min[i];
}

int mem_get_max(int i) {
  return servo_max[i];
}

#define FILENAME "/automaton.txt"

void mem_save(void) {

  InternalFS.remove(FILENAME);

  if (file.open(FILENAME, FILE_O_WRITE)) {
    Serial.println("OK");

    file.write(servo_min, FIN_NUM);
    file.write(servo_max, FIN_NUM);

    file.close();
  }
}

void mem_load(void) {
  if (file.open(FILENAME, FILE_O_READ)) {
    file.read(servo_min, FIN_NUM);
    file.read(servo_max, FIN_NUM);
    file.close();
  }
}

void mem_init(void) {
  InternalFS.begin();
  mem_load();
}

void mem_default(void) {
  for (int i = 0; i < FIN_NUM; i++) {
    servo_min[i] = 45;
    servo_max[i] = 125;
  }
  mem_save();
}

#define SERIAL_BUFFFER 100
static char serialinput[SERIAL_BUFFFER];
int serial_index = 0;

void mem_update(void) {


  if (Serial.available()) {
    uint8_t in = Serial.read();

    serialinput[serial_index++] = in;

    if (serial_index >= SERIAL_BUFFFER) {
      serial_index = 0;
    }

    serialinput[serial_index] = '\0';

    if (serialinput[serial_index - 1] == '\n') {
      Serial.println("Processing");
      serial_index = 0;

      int fin = -1;
      int min = -1;
      int max = -1;
      sscanf(serialinput, "%d %d %d", &fin, &min, &max);

      Serial.println(fin);
      Serial.println(min);
      Serial.println(max);

      if (max >= 0 && min >= 0 && fin >= 0 && fin < FIN_NUM && min <= 255 && max <= 255) {
        Serial.println("Updating....");
        if (min != 0)
          servo_min[fin] = min;
        if (max != 0)
          servo_max[fin] = max;
        mem_save();
      }
      static int fin_effect = 0;
      Serial.println("Fin Min & Max ");
      for (int i = 0; i < FIN_NUM; i++) {
        Serial.print(i);
        Serial.print('\t');
      }
      Serial.println(" ");
      for (int i = 0; i < FIN_NUM; i++) {
        Serial.print(servo_min[i]);
        Serial.print('\t');
      }
      Serial.println(" ");
      for (int i = 0; i < FIN_NUM; i++) {
        Serial.print(servo_max[i]);
        Serial.print('\t');
      }
      Serial.println(" ");
      Serial.println("Type FINNUM MIN MAX to change.");

      if (serialinput[0] == 'r') {
        Serial.println("Reset");
        mem_default();
      }
      if (serialinput[0] == 'z') {
        Serial.println("pos 0");
        fin_set(0, fin_effect);
      }
      if (serialinput[0] == 'x') {
        Serial.println("pos 1");
        fin_set(1, fin_effect);
      }
      if (serialinput[0] == 'c') {
        Serial.println("pos 2");
        fin_set(2, fin_effect);
      }
      if (serialinput[0] == 'v') {
        Serial.println("pos 3");
        fin_effect = 0;
      }
      if (serialinput[0] == 'b') {
        Serial.println("pos 4");
        fin_effect = 1;
      }
      if (serialinput[0] == 'n') {
        Serial.println("pos 5");
        fin_effect = 2;
      }
      if (serialinput[0] == 'm') {
        Serial.println("pos 5");
        fin_effect = 3;
      }
      if (serialinput[0] == ',') {
        Serial.println("bump down");
        fin_bump(1, 20);
      }
      if (serialinput[0] == '.') {
        Serial.println("bump down");
        fin_bump(-1, 20);
      }
      //new
      if (serialinput[0] == 'a') {
        Serial.println("hand roll -1");
        fin_smooth_left(-1);
      }
      if (serialinput[0] == 's') {
        Serial.println("hand roll 0");
        fin_smooth_left(0);
      }
      if (serialinput[0] == 'd') {
        Serial.println("hand roll 1");
        fin_smooth_left(1);
      }
      if (serialinput[0] == 'f') {
        Serial.println("hand roll -1");
        fin_smooth_right(-1);
      }
      if (serialinput[0] == 'g') {
        Serial.println("hand roll 0");
        fin_smooth_right(0);
      }
      if (serialinput[0] == 'h') {
        Serial.println("hand roll 1");
        fin_smooth_right(1);
      }
    }
  }
}