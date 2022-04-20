/*
   STUDENT: Erica Fenn
*/

/*
   InterruptLab (c) 2021-22 Christopher A. Bohn
*/

#include "cowpi.h"
#include "Math.h"

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
volatile unsigned long lastButtonPress = 0;
volatile uint8_t counter = 0;
volatile bool elapsedTime = false;


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
  0b01111110, 0b00110000, 0b01101101, 0b01111001, 0b00110011, 0b01011011, 0b01011111, 0b01110000,
  0b01111111, 0b01110011, 0b01110111, 0b00011111, 0b00001101, 0b00111101, 0b01001111, 0b01000111
};

uint8_t displaySegment[8] = {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000};

ISR(TIMER1_COMPA_vect) {
  if (elapsedTime) {
    counter++;
    if ((ioPorts[A0_A5].input & (1 << 4)) == 0) {
      if (counter == 40) {
        Serial.println("20 sec has elapsed");
        ioPorts[D8_D13].output &= ~(1 << 4);
        counter = 0;
        elapsedTime = 0;
        int n = 8;
        for (int i = 0; i < 8; i++) {
          displayData(n, displaySegment[i]);
          n--;
        }
      }
    } else {
      if (counter == 15) {
        Serial.println("7.5 sec has elapsed");
        ioPorts[D8_D13].output &= ~(1 << 4);
        counter = 0;
        elapsedTime = 0;
        int n = 8;
        for (int i = 0; i < 8; i++) {
          displayData(n, displaySegment[i]);
          n--;
        }
      }
    }
  }
}

void setup() {
  Serial.begin(9600);
  cowpi_setup(SPI | MAX7219);
  ioPorts = (cowpi_ioPortRegisters *) (cowpi_IObase + 3);
  spi = (cowpi_spiRegisters *) (cowpi_IObase + 0x2C);
  setupTimer();
  attachInterrupt(digitalPinToInterrupt (2), handleButtonAction , CHANGE );
  attachInterrupt(digitalPinToInterrupt (3), handleKeypress , CHANGE );
}

void loop() {
  // You can have code here while you're working on the assignment,
  // but be sure there isn't any code here by the time that you're finished.
  ;
}

void setupTimer() {
  TCCR1A = 0b00000000;
  TCCR1B = 0b00001101;
  OCR1A = 7812;
  TIMSK1 = 0b000000010;
}

volatile long leftButtonDown = 0;
volatile long leftButtonUp = 0;
volatile long rightButtonDown = 0;
volatile long rightButtonUp = 0;
volatile uint8_t leftPress = 0;
volatile uint8_t rightPress = 0;
volatile uint8_t oneClick = 0;
volatile long oneClickTime = 0;
volatile long value = 0;
volatile uint8_t dBuffer = 1;
volatile uint8_t numbers[8];

void handleButtonAction() {
  volatile uint8_t leftButton = digitalRead(8);
  volatile uint8_t rightButton = digitalRead(9);
  volatile unsigned long now = millis();
  if (now - lastButtonPress > DEBOUNCE_TIME) {
    lastButtonPress = now;
    if (!leftPress && !leftButton) {
      if (oneClick) {
        if (millis() - oneClickTime < 500) {
          Serial.println("Double-click");
          if ((ioPorts[A0_A5].input & (1 << 5)) == 0) {
            volatile uint8_t hex[8] = {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000};
            while (value != 0) {
              for (int x = 0; x < 8; x++) {
                if (hex[x] != 0) {
                  hex[x - 1] = hex[x];
                }
              }
              hex[7] = value % 16;
              value = value / 16;

              Serial.println(hex[7]);
              Serial.println(value);
            }
            int n = 8;
            for (int x = 0; x < 8; x++) {
              if (hex[x] != 0) {
                displayData(n, sevenSegments[hex[x]]);
              } else {
                displayData(n, 0b00000000);
              }
              n--;
            }
          } else {
            volatile double power = 7;
            volatile long decimal = 1;
            volatile uint8_t dec[8] = {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000};
            for (int i = 0; i < 8; i++) {
              if (numbers[i] != 0) {
                decimal += (numbers[i] * pow(double(16), double(power)));
              }
              power--;
            }
            int count = 7;
            while (decimal != 0) {
              dec[count] = decimal % 10;
              decimal = decimal / 10;
              count--;
            }
            int n = 8;
            for (int x = 0; x < 8; x++) {
              if (dec[x] != 0) {
                displayData(n, sevenSegments[dec[x]]);
              } else {
                displayData(n, 0b00000000);
              }
              n--;
            }
          }
          elapsedTime = true;
          ioPorts[D8_D13].output |= (1 << 4);
        }
        oneClick = 0;
      }
      leftPress = 1;
      leftButtonDown = millis();
    } else if (leftPress && leftButton && !oneClick) {
      if (millis() - leftButtonDown > 150) {
        Serial.println("Left button released");
        oneClick = 1;
        oneClickTime = millis();

        for (int i = 0; i < 8; i++) {
          if (displaySegment[i] != 0) {
            if (displaySegment[i] != 0b00000001) {
              displaySegment[7 - dBuffer] = 0b00000001;
              displayData(dBuffer, 0b00000001);
            } else {
              displaySegment[7 - dBuffer] = 0b00000000;
              displayData(dBuffer - 1, 0b00000000);
            }
          }
          value *= -1;
        }
        dBuffer++;
      }
      leftPress = 0;
    }

    if (!rightPress && !rightButton) {
      rightPress = 1;
      rightButtonDown = millis();
    } else if (rightPress && rightButton) {
      if (millis() - rightButtonDown > 150) {
        Serial.println("Right button released");

        for (int x = 1; x < 9; x++) {
          displayData(x, 0b00000000);
        }
        displayData(1, sevenSegments[0]);
        for (int x = 0; x < 8; x++) {
          displaySegment[x] = 0b00000000;
        }
        dBuffer = 1;
        value = 0;
      }
      rightPress = 0;
    }
  }
}

void handleKeypress() {
  uint8_t keyPressed = 0xFF;
  unsigned long now = millis();
  if (now - lastKeypadPress > DEBOUNCE_TIME) {
    lastKeypadPress = now;
    for (uint8_t x = 4; x > 0; x--) {
      uint8_t rowOutput = 0b11110000;
      rowOutput &= ~(1 << (x + 3));
      ioPorts[D0_D7].output = rowOutput;

      if ((ioPorts[A0_A5].input & 0b00000001) == 0) {
        keyPressed = keys[x - 1][0];
      } else if ((ioPorts[A0_A5].input & 0b00000010) == 0) {
        keyPressed = keys[x - 1][1];
      } else if ((ioPorts[A0_A5].input & 0b00000100) == 0) {
        keyPressed = keys[x - 1][2];
      } else if ((ioPorts[A0_A5].input & 0b00001000) == 0) {
        keyPressed = keys[x - 1][3];
      }
    }
    ioPorts[D0_D7].output &= 0b00001111;
  }

  volatile uint8_t rightSwitchCurrentPosition = ioPorts[A0_A5].input & (1 << 5);
  if (keyPressed < 0x10) {
    if (rightSwitchCurrentPosition == 0) {
      if (keyPressed != 0xA && keyPressed != 0xB && keyPressed != 0xC && keyPressed != 0xD &&
          keyPressed != 0xE && keyPressed != 0xF) {

        if (value == 0) {
          value += keyPressed;
        } else {
          value = (value * 10) + keyPressed;
        }
        if ((value > 99999999) | (value < -9999999)) {
          displayData(8, 0b00000000);
          displayData(7, 0b00001111);
          displayData(6, 0b00011101);
          displayData(5, 0b00011101);
          displayData(4, 0b00000000);
          displayData(3, 0b00011111);
          displayData(2, 0b00000110);
          displayData(1, 0b01011110);
        } else {
          volatile int count = dBuffer;
          for (int i = 1; i < 8; i++) {
            if (displaySegment[i] != 0) {
              displaySegment[i - 1] = displaySegment[i];
              displayData(count, displaySegment[i - 1]);
              count--;
            }
          }
          displaySegment[7] = sevenSegments[keyPressed];
          displayData(1, displaySegment[7]);
          dBuffer++;
        }
      }

    } else {

      if (value == 0) {
        value += keyPressed;
      } else {
        value = (value * 10) + keyPressed;
      }
      if ((value > 23666665) | (value < -800000000)) {
        displayData(8, 0b00000000);
        displayData(7, 0b00001111);
        displayData(6, 0b00011101);
        displayData(5, 0b00011101);
        displayData(4, 0b00000000);
        displayData(3, 0b00011111);
        displayData(2, 0b00000110);
        displayData(1, 0b01011110);
      } else {
        int counter = dBuffer;
        for (int i = 1; i < 8; i++) {
          if (displaySegment[i] != 0) {
            displaySegment[i - 1] = displaySegment[i];
            displayData(counter, displaySegment[i - 1]);
            counter--;
          }
        }
        displaySegment[7] = sevenSegments[keyPressed];
        displayData(1, displaySegment[7]);
        dBuffer++;

        for (int x = 0; x < 8; x++) {
          if (numbers[x] != 0) {
            numbers[x - 1] = numbers[x];
          }
        }
        numbers[7] = keyPressed;
      }
    }
  }
}

void displayData(uint8_t address, uint8_t value) {
  // address is MAX7219's register address (1-8 for digits; otherwise see MAX7219 datasheet Table 2)
  // value is the bit pattern to place in the register
  cowpi_spiEnable;
  ioPorts[D8_D13].output &= 0b11111011;
  spi->data = address;
  while (spi->status == 0) {
  }
  spi->data = value;
  while (spi->status == 0) {
  }
  ioPorts[D8_D13].output |= 0b00000100;
  cowpi_spiDisable;
}
