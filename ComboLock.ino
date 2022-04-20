/*
 * STUDENTS: Erica Fenn and Kyle Auman
 */

/*
 * CombinationLock GroupLab (c) 2022 Christopher A. Bohn
 */

#include <EEPROM.h>
#include "cowpi.h"

void setup() {
  Serial.begin(9600);
  cowpi_setup(SPI | MAX7219);
  cowpi_sendDataToMax7219(3, 0b1);
  cowpi_sendDataToMax7219(6, 0b1);
}

void loop() {
  int cursor = 2;
  if (cursor == 2) {
    cowpi_sendDataToMax7219(8, 0b10000000);
    cowpi_sendDataToMax7219(7, 0b10000000);
  } else if (cursor == 1) {
    cowpi_sendDataToMax7219(5, 0b10000000);
    cowpi_sendDataToMax7219(4, 0b10000000);
  } else {
    cowpi_sendDataToMax7219(2, 0b10000000);
    cowpi_sendDataToMax7219(1, 0b10000000);
  }
}

// 5 modes (LOCKED, UNLOCKED, ALARMED, CHANGING, CONFIRMING)

uint8_t getKeypress() {
  if (cowpi_getKeypress() == '1') {
    return 0x01;
  } else if (cowpi_getKeypress() == '2') {
    return 0x02;
  } else if (cowpi_getKeypress() == '3') {
    return 0x03;
  } else if (cowpi_getKeypress() == '4') {
    return 0x04;
  } else if (cowpi_getKeypress() == '5') {
    return 0x05;
  } else if (cowpi_getKeypress() == '6') {
    return 0x06;
  } else if (cowpi_getKeypress() == '7') {
    return 0x07;
  } else if (cowpi_getKeypress() == '8') {
    return 0x08;
  } else if (cowpi_getKeypress() == '9') {
    return 0x09;
  } else if (cowpi_getKeypress() == 'A') {
    return 0x0A;
  } else if (cowpi_getKeypress() == 'B') {
    return 0x0B;
  } else if (cowpi_getKeypress() == 'C') {
    return 0x0C;
  } else if (cowpi_getKeypress() == 'D') {
    return 0x0D;
  } else if (cowpi_getKeypress() == '#') {
    return 0x0E;
  } else if (cowpi_getKeypress() == '*') {
    return 0x0F;
  } else {
    return 0x00;
  }
}
