/*
 * ClearEEPROMPage (c) 2022 Christopher A. Bohn
 */

#include <EEPROM.h>

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < 2; i++) {
    Serial.print("In ");
    Serial.print(5 - 2 * i);
    Serial.println(" seconds you will be promted to enter an EEPROM address.");
    Serial.println("The address must be divisible by 4.");
    Serial.println("One page (4 bytes) of EEPROM memory will be cleared, starting at that address.");
    delay(2000);
  }
  Serial.println("In 1 second you will be prompted to enter an EEPROMM address.");
  delay(1000);
  unsigned long address;
  unsigned long value;
  do {
    Serial.print("Enter an EEPROM address divisible by 4 to clear one page (4 bytes) to 0: ");
    while(!Serial.available());
  } while (((address = Serial.parseInt()) % 4) || (address >= EEPROM.length()));
  Serial.println();
  Serial.print("Initial value at 0x");
  Serial.print(address, HEX);
  Serial.print(": 0x");
  EEPROM.get(address, value);
  Serial.println(value, HEX);
  value = 0x0L;
  Serial.print("Writing 0x00000000 to address 0x");
  Serial.println(address, HEX);
  EEPROM.put(address, value);
  Serial.print("New value at 0x");
  Serial.print(address, HEX);
  Serial.print(": 0x");
  EEPROM.get(address, value);
  Serial.println(value, HEX);
}

void loop() {
  ;
}
