/*
   STUDENTS: Erica Fenn and Kyle Auman
*/

/*
   CombinationLock GroupLab (c) 2022 Christopher A. Bohn
*/

#include <EEPROM.h>
#include "cowpi.h"

#define DEBOUNCE_TIME 300u
#define SINGLE_CLICK_TIME 150u
#define DOUBLE_CLICK_TIME 500u
#define NUMBER_OF_DIGITS 8

bool rightButtonIsPressed();

cowpi_ioPortRegisters *ioPorts;
unsigned long countdownStart = 0;
const uint8_t *message = NULL;
const uint8_t *lastMessage = NULL;

const uint8_t leftCursor[8] = {0, 0, 0x1, 0, 0, 0x1, 0x80, 0x80};
const uint8_t middleCursor[8] = {0, 0, 0x1, 0x80, 0x80, 0x1, 0, 0};
const uint8_t rightCursor[8] = {0x80, 0x80, 0x1, 0, 0, 0x1, 0, 0};
const uint8_t clearMessage[8] = {0, 0, 0x1, 0, 0, 0x1, 0, 0};

uint8_t display[8] = {0, 0, 0x1, 0, 0, 0x1, 0, 0};
uint8_t combo[8] = {0x30, 0x6D, 0x1, 0x79, 0x33, 0x1, 0x5B, 0x5F};
uint8_t errorMessage[8] = {0, 0, 0, 0x4F, 0x5, 0x5, 0x1D, 0x5};
uint8_t badTryMessage[8] = {0x1F, 0x77, 0x3D, 0xF, 0x5, 0x3B, 0, 0};
uint8_t alertMessage[8] = {0, 0, 0x77, 0xE, 0x4F, 0x5, 0xF, 0xA0};
uint8_t enterMessage[8] = {0, 0, 0, 0x4F, 0x76, 0xF, 0x4F, 0x5};
uint8_t reEnterMessage[8] = {0x5, 0x4F, 0x1, 0x4F, 0x76, 0xF, 0x4F, 0x5};
uint8_t confirmed[8] = {0, 0, 0x1, 0, 0, 0x1, 0, 0};
uint8_t changedMessage[8] = {0xD, 0x37, 0x77, 0x76, 0x5E, 0x4F, 0x3D};
uint8_t noChangeMessage[8] = {0x76, 0x1D, 0xD, 0x37, 0x77, 0x76, 0x5E, 0x4F};
uint8_t closedMessage[8] = {0, 0, 0xD, 0xE, 0x1D, 0x5B, 0x4F, 0x3D};
uint8_t labOpen[8] = {0xE, 0x77, 0x1F, 0, 0x1D, 0x67, 0x4F, 0x76};

bool equal = false;
int attempt = 0;
bool alarm = false;
unsigned long lastKeypadPress = 0;
unsigned long lastButtonPress = 0;
int mode = 1;
uint8_t leftSwitch = ioPorts[A0_A5].input & (1 << 4);
uint8_t rightSwitch = ioPorts[A0_A5].input & (1 << 5);

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
  ioPorts = (cowpi_ioPortRegisters *) (cowpi_IObase + 3);
  cowpi_sendDataToMax7219(3, 0x1);
  cowpi_sendDataToMax7219(6, 0x1);
  message = leftCursor;
}

int cursor = 2;
void loop() {
  leftSwitch = ioPorts[A0_A5].input & (1 << 4);
  rightSwitch = ioPorts[A0_A5].input & (1 << 5);
  unsigned long now = millis();
  handleKeypress(now);
  responsiveMessageWithoutInterrupts(now);

  if (mode == 2) {
    unlockMode();
  } else if (mode == 3) {
    changingMode();
  } else if (mode == 4) {
    confirmingMode();
  } else if (mode == 5) {
    lockedMode();
  }

  if ((mode == 1) && (digitalRead(8) == 0)) {
    leftButtonPress();
  }

}

int currentPosition = 7;
uint8_t currentMessage[8] = {0,0,0x1,0,0,0x1,0x80,0x80};
uint8_t defaultMessage[8] = {0,0,0x1,0,0,0x1,0,0};

void responsiveMessageWithoutInterrupts(unsigned long now) {
  if (now - lastButtonPress > DEBOUNCE_TIME) {
    lastButtonPress = now;
    handleRightButtonPress();
  } else {
    now = millis();
    if (now - countdownStart > 1000) {
      countdownStart = now;
      for(int i=0; i<8; i++) {
        Serial.print(message[i]);
        Serial.print(", ");
      }
      Serial.println("message");
      for(int i=0; i<8; i++) {
        Serial.print(defaultMessage[i]);
        Serial.print(", ");
      }
      Serial.println("defaultMessage");
      for(int i=0; i<8; i++) {
        Serial.print(lastMessage[i]);
        Serial.print(", ");
      }
      Serial.println("lastMessage");
      if (message == defaultMessage) {
        Serial.println("Message update 1");
        message = lastMessage;
        lastMessage = defaultMessage;
      } else {
        Serial.println("Message update 2");
        lastMessage = message;
        message = defaultMessage;
      }
      if (message != NULL) {
        displayMessage(message);
      }
    }
  }
}

void handleRightButtonPress() {
  if (digitalRead(9) != 1) {
    countdownStart = millis();
    if (currentPosition < 2) {
      currentPosition = 7;
      currentMessage[1] &= 0x7f;
      currentMessage[0] &= 0x7f;
      copyArray(currentMessage, defaultMessage);
      defaultMessage[7] = 0;
      defaultMessage[6] = 0;
      currentMessage[7] |= 0x80;
      currentMessage[6] |= 0x80;
      lastMessage = currentMessage;
      message = defaultMessage;
    } else if (currentPosition < 5) {
      currentPosition = 1;
      currentMessage[4] &= 0x7f;
      currentMessage[3] &= 0x7f;
      copyArray(currentMessage, defaultMessage);
      defaultMessage[1] = 0;
      defaultMessage[0] = 0;
      currentMessage[1] |= 0x80;
      currentMessage[0] |= 0x80;
      lastMessage = currentMessage;
      message = defaultMessage;
    } else {
      currentPosition = 4;
      currentMessage[7] &= 0x7f;
      currentMessage[6] &= 0x7f;
      copyArray(currentMessage, defaultMessage);
      defaultMessage[4] = 0;
      defaultMessage[3] = 0;
      currentMessage[4] |= 0x80;
      currentMessage[3] |= 0x80;
      lastMessage = currentMessage;
      message = defaultMessage;
    }
  }
}

void handleKeypress(unsigned long now) {
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
  }
}

void copyArray(uint8_t source[8], uint8_t destination[8]) {
  for(int i=0; i<8; i++) {
    destination[i] |= source[i];
  }
}

void displayMessage(const uint8_t message[8]) {
  for (int i = 8; i > 0; i--) {
    cowpi_sendDataToMax7219(i, message[i - 1]);
  }
}

void leftButtonPress() {
  for (int i = 0, j = 8; i < 8 && j > 0; i++, j--) {
    if ((currentMessage[j-1] & 0x7F) == 0) {
      int displayNum = 8;
      for (int count = 0; count < 8; count++) {
        cowpi_sendDataToMax7219(displayNum, errorMessage[count]);
        displayNum--;
      }
      delay(1000);
      int count = 8;
      for (int x = 0; x < 8; x++) {
        cowpi_sendDataToMax7219(count, (currentMessage[x] & 0x7F));
        count--;
      }
      i = 7;
    } else if ((currentMessage[j-1] & 0x7F) == combo[i]) {
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
        delay(1000);
      } else if (attempt == 2) {
        cowpi_sendDataToMax7219(1, 0b01101101);
        delay(1000);
      } else if (attempt == 3) {
        cowpi_sendDataToMax7219(1, 0b01111001);
        delay(500);
        callAlarm();
      }
      i = 7;
    }
  }
  if (equal == true) {
    int displayNum = 8;
    for (int count = 0; count < 8; count++) {
      cowpi_sendDataToMax7219(displayNum, labOpen[count]);
      displayNum--;
    }
    delay(1000);
    mode = 2;
  }
}

void callAlarm() {
  alarm = true;
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

void unlockMode() {
  if ((leftSwitch != 0) && (rightSwitch != 0) && (digitalRead(8) == 0)) {
    mode = 3;
    int displayNum = 8;
    for (int count = 0; count < 8; count++) {
      cowpi_sendDataToMax7219(displayNum, enterMessage[count]);
      displayNum--;
    }
    delay(1000);
  }

  if ((leftSwitch == 0) && (rightSwitch == 0) && (digitalRead(8) == 0) && (digitalRead(9) == 0)) {
    mode = 5;
  }
}

void changingMode() {
  // int count = 8;
  // for (int x = 0; x < 8; x++) {
  //   cowpi_sendDataToMax7219(count, clearMessage[x]);
  //   count--;
  // }
  // only blinks if message != defaultMessage != lastMessage
  currentPosition = 7;
  // message = leftCursor;
  lastMessage = clearMessage;
  // copyArray(leftCursor, currentMessage);
  // copyArray(clearMessage, defaultMessage);
  copyArray(leftCursor, defaultMessage);
  defaultMessage[3] = 0;
  defaultMessage[4] = 0;
  defaultMessage[6] = 0x80;
  defaultMessage[7] = 0x80;
  message = defaultMessage;
  
  //read numbers being pressed and put into display array
  if ((leftSwitch == 0) && (digitalRead(8) == 0)) {
    mode = 4;
    int displayNum = 8;
    for (int count = 0; count < 8; count++) {
      cowpi_sendDataToMax7219(displayNum, reEnterMessage[count]);
      displayNum--;
    }
    delay(1000);
  }
}

void confirmingMode() {
  int count = 8;
  for (int x = 0; x < 8; x++) {
    cowpi_sendDataToMax7219(count, clearMessage[x]);
    count--;
  }
  //read numbers being pressed and put into a new array
  if ((rightSwitch == 0) && (digitalRead(8) == 0)) {
    equal = false;
    for (int x = 0; x < 8; x++) {
      if (confirmed[x] == (currentMessage[x] & 0x7F)) {
        equal = true;
      } else {
        equal = false;
        x = 7;
        int displayNum = 8;
        for (int count = 0; count < 8; count++) {
          cowpi_sendDataToMax7219(displayNum, noChangeMessage[count]);
          displayNum--;
        }
        delay(1000);
      }
    }
    if (equal == true) {
      int displayNum = 8;
      for (int count = 0; count < 8; count++) {
        cowpi_sendDataToMax7219(displayNum, changedMessage[count]);
        displayNum--;
      }
      for (int count = 0; count < 8; count++) {
        combo[count] = (currentMessage[count] & 0x7F);
      }
      mode = 2;
    }
  }
}

void lockedMode() {
  int displayNum = 8;
  for (int count = 0; count < 8; count++) {
    cowpi_sendDataToMax7219(displayNum, closedMessage[count]);
    displayNum--;
  }
  delay(1000);
  int i = 8;
  for (int x = 0; x < 8; x++) {
    cowpi_sendDataToMax7219(i, clearMessage[x]);
    i--;
  }
  mode = 1;
}
