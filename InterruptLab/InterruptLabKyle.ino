/*
 * STUDENT: Kyle Auman
 */

/*
 * InterruptLab (c) 2021-22 Christopher A. Bohn
 */

#include "cowpi.h"

#define DEBOUNCE_TIME 20u
#define SINGLE_CLICK_TIME 150u
#define DOUBLE_CLICK_TIME 500u
#define NUMBER_OF_DIGITS 8

void setupTimer();
void handleButtonAction();
void handleKeypress();
void displayData(uint8_t address, uint8_t value);

/* Memory-mapped I/O */
cowpi_ioPortRegisters *ioPorts;     // an array of I/O ports
cowpi_spiRegisters *spi;            // a pointer to the single set of SPI registers
cowpi_timerRegisters16bit *timer1;  // a pointer to one 16-bit timer
cowpi_timerRegisters8bit *timer2;   // a pointer to one 8-bit timer

/* Variables for software debouncing */
volatile unsigned long lastLeftButtonAction = 0;
volatile unsigned long lastRightButtonAction = 0;
volatile unsigned long lastLeftSwitchSlide = 0;
volatile unsigned long lastRightSwitchSlide = 0;
volatile unsigned long lastKeypadPress = 0;


// Layout of Matrix Keypad
//        1 2 3 A
//        4 5 6 B
//        7 8 9 C
//        * 0 # D
// This array holds the values we want each keypad button to correspond to
const uint8_t keys[4][4] = {
  {0x1, 0x2, 0x3, 0xA},
  {0x4, 0x5, 0x6, 0xB},
  {0x7, 0x8, 0x9, 0xC},
  {0xF, 0x0, 0xE, 0xD}
};

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
  ioPorts = (cowpi_ioPortRegisters *)(cowpi_IObase + 3);
  spi = (cowpi_spiRegisters *)(cowpi_IObase + 0x2C);
  setupTimer();
  attachInterrupt(digitalPinToInterrupt(2), handleButtonAction, CHANGE);
  attachInterrupt(digitalPinToInterrupt(3), handleKeypress, CHANGE);
}

#define BUTTON_NO_REPEAT_TIME 500u
void loop() {
  // You can have code here while you're working on the assignment,
  // but be sure there isn't any code here by the time that you're finished.
}

ISR(TIMER1_COMPA_vect) {

}

void setupTimer() {
  timer1 = (cowpi_timerRegisters16bit *)(cowpi_IObase + 0x60);
  timer1->control |= 0b0000100000000000;
  timer1->compareA = 0x0002;
}

void handleButtonAction() {
  uint8_t printedThisTime = 0;
  uint8_t rightButtonCurrentPosition = digitalRead(9);
  unsigned long now = millis();
  if (!rightButtonCurrentPosition && (now - lastRightButtonAction > BUTTON_NO_REPEAT_TIME)) {
    Serial.println("\tData Cleared");
    lastRightButtonAction = now;
    clearData();
  }
}

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

int address = 1;

void handleKeypress() {
  unsigned long now = millis();
  if (cowpi_getKeypress() && now - lastKeypadPress > DEBOUNCE_TIME) {
    uint8_t keypress = getKeypress();
    lastKeypadPress = now;
    if (keypress < 0x10) {
      Serial.print("Key pressed: ");
      Serial.println(keypress, HEX);
      if (digitalRead(A4) == 0) {
        if (digitalRead(A5) != 0) {
          if (getKeypress() < 0xA) {
            displayData(1, sevenSegments[keypress]);
          }
        } else {
          displayData(1, sevenSegments[keypress]);
        }
      } else {
        if (digitalRead(A5) != 0) {
          if (getKeypress() < 0xA) {
            displayData(address, sevenSegments[keypress]);
          }
        } else {
          displayData(address, sevenSegments[keypress]);
        }
        address++;
        if (address > 8) {
          // display too big
        }
      }
    } else {
      Serial.println("Error reading keypad.");
    }
  }
}

void displayData(uint8_t address, uint8_t value) {
  // address is MAX7219's register address (1-8 for digits; otherwise see MAX7219 datasheet Table 2)
  // value is the bit pattern to place in the register
  cowpi_sendDataToMax7219(address, value);
}

void clearData() {
  for (int i=0; i<9; i++) {
    cowpi_sendDataToMax7219(i, 0x00);
  }
  cowpi_sendDataToMax7219(1, sevenSegments[0]);
}
