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

bool rightButtonIsPressed();

unsigned long countdownStart = 0;
const uint8_t *message = NULL;
const uint8_t *lastMessage = NULL;

const uint8_t leftCursor[8] = {0,0,0x1,0,0,0x1,0x80,0x80};
const uint8_t middleCursor[8] = {0,0,0x1,0x80,0x80,0x1,0,0};
const uint8_t rightCursor[8] = {0x80,0x80,0x1,0,0,0x1,0,0};
const uint8_t clearMessage[8] = {0,0,0x1,0,0,0x1,0,0};

uint8_t display[8] = {0b00000000,0b00000000,0b00000001,0b00000000,0b00000000,0b00000001,0b00000000,0b00000000};
uint8_t combo[8] = {0b00110000, 0b01101101, 0b00000001, 0b01111001, 0b00110011, 0b00000001, 0b01011011, 0b01011111};
uint8_t errorMessage[8] = {0b00000000,0b00000000,0b00000000,0b01001111,0b00000101,0b00000101,0b00011101,0b00000101};
uint8_t badTryMessage[8] = {0b00011111, 0b01110111, 0b00111101,0b00001111,0b00000101,0b00111011,0b00000000,0b00000000};
uint8_t alertMessage[8] = {0b00000000,0b00000000, 0b01110111,0b00001110,0b01001111,0b00000101,0b00001111,0b10100000};

bool equal = false;
int attempt = 0;
bool alarm = false;
unsigned long lastKeypadPress = 0;
unsigned long lastButtonPress = 0;

void setup() {
  Serial.begin(9600);
  cowpi_setup(SPI | MAX7219);
  cowpi_sendDataToMax7219(3, 0x1);
  cowpi_sendDataToMax7219(6, 0x1);
  message = leftCursor;
}

int cursor = 2;
void loop() {
  handleKeypress();
  responsiveMessageWithoutInterrupts();

  if (digitalRead(8) == 0 ) {
    leftButtonPress();
  }
}

// 5 modes (LOCKED, UNLOCKED, ALARMED, CHANGING, CONFIRMING)

void responsiveMessageWithoutInterrupts() {
  volatile unsigned long now = millis();
  if (now - lastButtonPress > DEBOUNCE_TIME) {
    lastButtonPress = now;
    if (digitalRead(9) != 1) {
      countdownStart = millis();
      if (cursor == 2) {
        cursor--;
        message = middleCursor;
        lastMessage = clearMessage;
        displayMessage(message);
      } else if (cursor == 1) {
        cursor--;
        message = rightCursor;
        lastMessage = clearMessage;
        displayMessage(message);
      } else if (cursor == 0) {
        cursor = 2;
        message = leftCursor;
        lastMessage = clearMessage;
        displayMessage(message);
      }
    }
  } else {
    now = millis();
    if (now - countdownStart > 1000) {
      countdownStart = now;
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

void displayMessage(const uint8_t message[8]) {
  for (int i=8; i>0; i--) {
    cowpi_sendDataToMax7219(i, message[i-1]);
  }
}

void leftButtonPress() {
  // submits Combo
  // if all numbers not entered, display error for one second
  for (int i = 0; i < 8; i++) {
    if (display[i] == 0) {
      int displayNum = 8;
      for (int count = 0; count < 8; count++) {
        cowpi_sendDataToMax7219(displayNum, errorMessage[count]);
        displayNum--;
      }
      delay(1000);
      int count = 8;
      for (int x = 0; x < 8; x++) {
        cowpi_sendDataToMax7219(count, display[x]);
        count--;
      }
      i =7;
    } else if (display[i] == combo[i]) {
        equal = true;
    } else {
      equal = false;
      attempt++;
      int displayNum = 8;
      for (int count = 0; count < 8; count++) {
        cowpi_sendDataToMax7219(displayNum, badTryMessage[count]);
        displayNum--;
      }
      if (attempt == 1) {
        cowpi_sendDataToMax7219(1, 0b00110000);
      } else if (attempt == 2) {
        cowpi_sendDataToMax7219(1, 0b01101101);
      } else {
        alarm = true;
        cowpi_sendDataToMax7219(1, 0b01111001);
        int displayNum = 8;
        for (int count = 0; count < 8; count++) {
          cowpi_sendDataToMax7219(displayNum, alertMessage[count]);
          displayNum--;
        }
        while (alarm == true) {
          digitalWrite(12, HIGH);
          delay(250);
          digitalWrite(12, LOW);
          delay(250);
        }
      }
      i = 7;
    }
  }
    if (equal == true) {
      cowpi_sendDataToMax7219(8, 0b00001110);
      cowpi_sendDataToMax7219(7, 0b01110111);
      cowpi_sendDataToMax7219(6, 0b00011111);
      cowpi_sendDataToMax7219(5, 0b00000000);
      cowpi_sendDataToMax7219(4, 0b00011101);
      cowpi_sendDataToMax7219(3, 0b01100111);
      cowpi_sendDataToMax7219(2, 0b01001111);
      cowpi_sendDataToMax7219(2, 0b01110110);
    }
}
