/*
 * STUDENTS: Erica Fenn and Kyle Auman
 */

/*
 * CombinationLock GroupLab (c) 2022 Christopher A. Bohn
 */

#include <EEPROM.h>
#include "cowpi.h"

#define DEBOUNCE_TIME 300u
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

bool rightButtonIsPressed();

unsigned long countdownStart = 0;
const uint8_t *message = NULL;
const uint8_t *lastMessage = NULL;

const uint8_t leftCursor[8] = {0,0,0x1,0,0,0x1,0x80,0x80};
const uint8_t middleCursor[8] = {0,0,0x1,0x80,0x80,0x1,0,0};
const uint8_t rightCursor[8] = {0x80,0x80,0x1,0,0,0x1,0,0};
uint8_t clearMessage[8] = {0,0,0x1,0,0,0x1,0,0};
// uint8_t defaultMessage[8] = {0,0,0x1,0,0,0x1,0,0};

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

void setup() {
  Serial.begin(9600);
  cowpi_setup(SPI | MAX7219);
  cowpi_sendDataToMax7219(3, 0x1);
  cowpi_sendDataToMax7219(6, 0x1);
  message = leftCursor;
}

void loop() {
  unsigned long now = millis();
  handleKeypress(now);
  responsiveMessageWithoutInterrupts(now);
}

// 5 modes (LOCKED, UNLOCKED, ALARMED, CHANGING, CONFIRMING)

int currentPosition = 7;
uint8_t currentMessage[8] = {0,0,0x1,0,0,0x1,0x80,0x80};

void responsiveMessageWithoutInterrupts(unsigned long now) {
  // volatile unsigned long now = millis();
  if (now - lastButtonPress > DEBOUNCE_TIME) {
    lastButtonPress = now;
    // if (digitalRead(9) != 1) {
    //   countdownStart = millis();
    //   if (currentPosition < 2) {
    //     currentPosition = 7;
    //     message = leftCursor;
    //     lastMessage = clearMessage;
    //   } else if (currentPosition < 5) {
    //     currentPosition = 1;
    //     message = rightCursor;
    //     lastMessage = clearMessage;
    //   } else {
    //     currentPosition = 4;
    //     currentMessage[7] &= 0x7f;
    //     currentMessage[6] &= 0x7f;
    //     lastMessage = currentMessage;
    //     currentMessage[4] = middleCursor[4];
    //     currentMessage[3] = middleCursor[3];
    //     message = currentMessage;
    //   }
    // }
    if (digitalRead(9) != 1) {
      countdownStart = millis();
      if (currentPosition < 2) {
        currentPosition = 7;
        message = leftCursor;
        lastMessage = clearMessage;
        displayMessage(message);
      } else if (currentPosition < 5) {
        currentPosition = 1;
        message = rightCursor;
        lastMessage = clearMessage;
        displayMessage(message);
      } else {
        currentPosition = 4;
        message = middleCursor;
        lastMessage = clearMessage;
        displayMessage(message);
      }
    }
  } else {
    now = millis();
    if (now - countdownStart > 1000) {
      countdownStart = now;
      // changes need to be made to this to correctly display the message
      if (message == clearMessage) {
        message = lastMessage;
        lastMessage = clearMessage;
      } else {
        lastMessage = message;
        message = clearMessage;
      }
      if (message != NULL) {
        displayMessage(message);
      }
    }
    // if (now - countdownStart > 1000) {
    //   countdownStart = now;
    //   if (message == clearMessage) {
    //     message = lastMessage;
    //     lastMessage = clearMessage;
    //   } else {
    //     lastMessage = message;
    //     message = clearMessage;
    //   }
    //   if (message != NULL) {
    //     displayMessage(message);
    //   }
    // }
  }
}

uint8_t enterLeftMessage[8] = {0,0,0x1,0,0,0x1,0x80,0x80};
uint8_t enterMiddleMessage[8] = {0,0,0x1,0x80,0x80,0x1,0,0};
uint8_t enterRightMessage[8] = {0x80,0x80,0x1,0,0,0x1,0,0};

void handleKeypress(unsigned long now) {
  // unsigned long now = millis();
  if (cowpi_getKeypress() && now - lastKeypadPress > DEBOUNCE_TIME) {
    countdownStart = millis();
    lastKeypadPress = now;
    uint8_t keypress = sevenSegments[getKeypress()];
    if (currentPosition == 7) {
      currentMessage[7] = keypress | 0x80;
      currentPosition--;
    } else if (currentPosition == 6) {
      currentMessage[6] = keypress | 0x80;
      currentPosition++;
    } else if (currentPosition == 4) {
      currentMessage[4] = keypress | 0x80;
      currentPosition--;
    } else if (currentPosition == 3) {
      currentMessage[3] = keypress | 0x80;
      currentPosition++;
    } else if (currentPosition == 1) {
      currentMessage[1] = keypress | 0x80;
      currentPosition--;
    } else {
      currentMessage[0] = keypress | 0x80;
      currentPosition++;
    }
    message = currentMessage;
    if (keypress < 0x10) {
      Serial.print("Key pressed: ");
      Serial.println(keypress, HEX);
    } else {
      Serial.println("Error reading keypad.");
      Serial.println(currentPosition);
    }
  }
}

void displayMessage(const uint8_t message[8]) {
  for (int i=8; i>0; i--) {
    cowpi_sendDataToMax7219(i, message[i-1]);
  }
}
