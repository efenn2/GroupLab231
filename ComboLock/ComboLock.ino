/*
 * STUDENTS: Erica Fenn and Kyle Auman
 */

/*
 * CombinationLock GroupLab (c) 2022 Christopher A. Bohn
 */

#include <EEPROM.h>
#include "cowpi.h"

#define DEBOUNCE_TIME 20u
#define SINGLE_CLICK_TIME 150u
#define DOUBLE_CLICK_TIME 500u
#define NUMBER_OF_DIGITS 8

/* Memory-mapped I/O */
cowpi_ioPortRegisters *ioPorts;     // an array of I/O ports
cowpi_spiRegisters *spi;            // a pointer to the single set of SPI registers
cowpi_timerRegisters16bit *timer1;  // a pointer to one 16-bit timer
cowpi_timerRegisters8bit *timer2;   // a pointer to one 8-bit timer

volatile unsigned long lastLeftButtonAction = 0;
volatile unsigned long lastRightButtonAction = 0;
volatile unsigned long lastLeftSwitchSlide = 0;
volatile unsigned long lastRightSwitchSlide = 0;
volatile unsigned long lastKeypadPress = 0;
volatile unsigned long lastButtonPress = 0;

// Seven Segment Display mapping between segments and bits
// Bit7 Bit6 Bit5 Bit4 Bit3 Bit2 Bit1 Bit0
//  DP   A    B    C    D    E    F    G
// This array holds the bit patterns to display each hexadecimal numeral
const uint8_t sevenSegments[16] = {
  0b1111110,  // 0
  0b0110000,  // 1
  0b1101101,  // 2
  0b1111001,  // 3
  0b0110011,  // 4
  0b1011011,  // 5
  0b1011111,  // 6
  0b1110000,  // 7
  0b1111111,  // 8
  0b1111011,  // 9
  0b1110111,  // A
  0b0011111,  // b
  0b0001101,  // c
  0b0111101,  // d
  0b1001111,  // E
  0b1000111   // F
};

void setup() {
  Serial.begin(9600);
  cowpi_setup(SPI | MAX7219);
  cowpi_sendDataToMax7219(3, 0x1);
  cowpi_sendDataToMax7219(6, 0x1);
}

int cursor = 2;

void loop() {
  volatile unsigned long now = millis();
  if (now - lastButtonPress > DEBOUNCE_TIME) {
    lastButtonPress = now;
    if (digitalRead(9) != 1) {
      if (cursor == 0) {
        cursor = 2;
      } else {
        cursor--;
      }
    }
  }
  
  if (cursor == 2) {
    cowpi_sendDataToMax7219(2, 0x00);
    cowpi_sendDataToMax7219(1, 0x00);

    cowpi_sendDataToMax7219(8, 0x80);
    cowpi_sendDataToMax7219(7, 0x80);
    delay(500);
    cowpi_sendDataToMax7219(8, 0x00);
    cowpi_sendDataToMax7219(7, 0x00);
    delay(500);
    handleKeypress();
  } else if (cursor == 1) {
    cowpi_sendDataToMax7219(8, 0x00);
    cowpi_sendDataToMax7219(7, 0x00);

    cowpi_sendDataToMax7219(5, 0x80);
    cowpi_sendDataToMax7219(4, 0x80);
    delay(500);
    cowpi_sendDataToMax7219(5, 0x00);
    cowpi_sendDataToMax7219(4, 0x00);
    delay(500);
  } else {
    cowpi_sendDataToMax7219(5, 0x00);
    cowpi_sendDataToMax7219(4, 0x00);

    cowpi_sendDataToMax7219(2, 0x80);
    cowpi_sendDataToMax7219(1, 0x80);
    delay(500);
    cowpi_sendDataToMax7219(2, 0x00);
    cowpi_sendDataToMax7219(1, 0x00);
    delay(500);
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

void handleKeypress() {
  unsigned long now = millis();
  if (cowpi_getKeypress() && now - lastKeypadPress > DEBOUNCE_TIME) {
    uint8_t keypress = getKeypress();
    lastKeypadPress = now;
    if (keypress < 0x10) {
      Serial.print("Key pressed: ");
      Serial.println(keypress, HEX);
    } else {
      Serial.println("Error reading keypad.");
    }
  }
}
