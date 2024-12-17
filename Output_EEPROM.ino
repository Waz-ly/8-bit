#define li long long int

#define SHIFT_DATA 2
#define SHIFT_CLK 3
#define BIG_CHIP_WRITE_EN 4
#define EEPROM_D0 5
#define EEPROM_D7 12
#define WRITE_EN 13

char hexmap[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

int sevenSegment[10] = { 0b00111111, 0b00001100, 0b01011011, 0b01011110, 0b01011100, 0b01110110, 0b01110111, 0b00011100, 0b01111111, 0b01111100 };

/*
 * Output the address bits and outputEnable signal using shift registers.
 */
void setAddress(int address, bool outputEnable) {
  // shift in output Enable to bit 13
  if(outputEnable) {
    digitalWrite(SHIFT_DATA, LOW);
  } else {
    digitalWrite(SHIFT_DATA, HIGH);
  }

  digitalWrite(SHIFT_CLK, HIGH);
  digitalWrite(SHIFT_CLK, LOW);

  // shift in the address
  for(int i = 12; i >= 0; i--) {
    if((address >> i) & 0b1) {
      digitalWrite(SHIFT_DATA, HIGH);
    } else {
      digitalWrite(SHIFT_DATA, LOW);
    }

    digitalWrite(SHIFT_CLK, HIGH);
    digitalWrite(SHIFT_CLK, LOW);
  }
}


/*
 * Read a byte from the EEPROM at the specified address.
 */
int readEEPROM(int address) {

  setAddress(address, /*outputEnable*/ true);

  int data = 0;
  
  for (int pin = EEPROM_D7; pin >= EEPROM_D0; pin -= 1) {
    pinMode(pin, INPUT);
    data = (data << 1) + digitalRead(pin);
  }
  
  return data;
}


/*
 * Write a byte to the EEPROM at the specified address.
 */
void writeEEPROM(int address, int data) {
  
  setAddress(address, /*outputEnable*/ false);

  for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin += 1) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, data & 0b01);
    data >>= 1;
  }
  
  digitalWrite(WRITE_EN, LOW);
  delayMicroseconds(1);
  digitalWrite(WRITE_EN, HIGH);
  
  delay(20);
}


/*
 * Read the contents of the EEPROM and print them to the serial monitor.
 */
void printContents(int start, int printLength) {
  
  for (int base = start; base < printLength; base += 4) {
    
    int data[4];
    
    for (int offset = 0; offset < 4; offset += 1) {
      data[offset] = readEEPROM(base + offset);
    }

    for (int offset = 0; offset < 4; offset++) {
      int dataStored = (int) data[offset];
      Serial.print(base + offset);
      Serial.print("::");
      Serial.print(hexmap[dataStored/16]);
      Serial.print(hexmap[dataStored%16]);
      Serial.print(" ");
    }
    
    Serial.print("\n");
  }
}

void writeCodeToEEPROM() {

  int chipSelect;
  for (int address = 0; address < 1024; address++) {
    chipSelect = (address >> 8)%4;
    int addressResidue = address%256;

    if(chipSelect == 3) {
      writeEEPROM(address, sevenSegment[addressResidue%10]);
    } else if(chipSelect == 2) {
      writeEEPROM(address, sevenSegment[(addressResidue/10)%10]);
    } else {
      writeEEPROM(address, sevenSegment[(addressResidue/100)%10]);
    }
  }
}

void setup() {

  // setup
  pinMode(SHIFT_DATA, OUTPUT);
  pinMode(SHIFT_CLK, OUTPUT);
  pinMode(BIG_CHIP_WRITE_EN, OUTPUT);
  digitalWrite(WRITE_EN, HIGH);
  pinMode(WRITE_EN, OUTPUT);
  digitalWrite(BIG_CHIP_WRITE_EN, LOW);
  Serial.begin(9600);

  // write to EEPROM
  writeCodeToEEPROM();
  
  // Read and print out the contents of the EERPROM
  Serial.println("\nReading EEPROM...");
  printContents(0, 2048);
  Serial.println("done.\n-----------------------------------------------");
}


void loop() {
  // put your main code here, to run repeatedly:

}
