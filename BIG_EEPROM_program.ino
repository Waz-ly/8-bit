#define SHIFT_DATA 2
#define SHIFT_CLK 3
#define WRITE_EN 4
#define EEPROM_D0 5
#define EEPROM_D7 12
#define EEPROM_A12 13

char hexmap[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

/*
 * m = move
 * ld = load
 * sm = sum
 * sb = subtraction
 * ad = and
 * ji/jmp = jump if/jump
 * nop = no operation
 * 
 * A = A register
 * B = B register
 * R = RAM
 * Z = Zero
 * C = Carry
 */

#define mAB   0 
#define mAR   1
#define mBA   2
#define mBR   3
#define mRA   4
#define mRB   5
#define ldA   6
#define ldB   7
#define smA   8
#define smB   9
#define sbA   10
#define sbB   11
#define adA   12
#define adB   13
#define jmp   14
#define jiZ   15
#define jiC   16
#define out   17
#define nop   18

/*
 * RESERVED RAM LOCATIONS
 * addresses 0 and 1 store where to jump back to
 * addresses 2 - 15 will all be used by subroutines
 * output onto address 15
 * inputs generally starting from 2
 */

#define startLine 64
#define startline 64
#define start_instructions (110 - startLine)
#define multiply_setup (71 - startLine)
#define multiply_loop (75 - startLine)
#define divide_setup (90 - startLine)
#define divide_loop (94 - startLine)
#define NA 0

int instructions[] = {
  nop, NA,
  ldA, 0,
  out, NA,
  ldA, (start_instructions)%256,
  ldB, (start_instructions)/256,
  jmp, NA,
  nop, NA, //****************//
  ldA, 1, // multiply setup
  ldB, 0,
  mBR, 15,
  smA, NA,
  mRA, 15, // multiply loop
  mRB, 14,
  smA, NA,
  mAR, 15,
  nop, NA,
  mRA, 13,
  ldB, 1,
  sbA, NA,
  mAR, 13,
  mRA, 0,
  mRB, 1,
  jiZ, NA,
  ldA, (multiply_loop)%256,
  ldB, (multiply_loop)/256,
  jmp, NA,
  nop, NA, //****************//
  mRA, 2, // divide setup
  mAR, 14,
  ldB, 0,
  mBR, 15,
  mRA, 15, // divide loop
  ldB, 1,
  smA, 0,
  mAR, 15,
  mRA, 14,
  mRB, 3,
  sbA, NA,
  mAR, 14,
  mRA, 0,
  mRB, 1,
  jiC, NA,
  ldA, (divide_loop)%256,
  ldB, (divide_loop)/256,
  jmp, NA,
  nop, NA, //****************//
  ldA, 17,
  mAR, 13,
  ldA, 13,
  mAR, 14,
  ldA, (121-startLine)%256,
  mAR, 0,
  ldA, (121-startLine)/256,
  mAR, 1,
  ldA, multiply_setup%256,
  ldB, multiply_setup/256,
  jmp, NA,
  nop, NA,
  mRA, 15,
  out, NA,
  nop, NA,
  nop, NA,
  nop, NA
};

int instructionSize = sizeof(instructions)/sizeof(instructions[0]);

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

  if((address >> 12) & 1) {
    digitalWrite(EEPROM_A12, HIGH);
  } else {
    digitalWrite(EEPROM_A12, LOW);
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
    digitalWrite(pin, data & 0b1);
    data >>= 1;
  }
  
  digitalWrite(WRITE_EN, LOW);
  delayMicroseconds(1);
  digitalWrite(WRITE_EN, HIGH);
  
  delay(30);
}


/*
 * Read the contents of the EEPROM and print them to the serial monitor.
 */
void printContents(int start, int printLength) {
  
  for (int base = start; base < printLength; base += 1) {
    
    int data;
    data = readEEPROM(base);

    int dataStored = data;
    Serial.print(base);
    Serial.print("::");
    Serial.print(hexmap[dataStored/16]);
    Serial.print(hexmap[dataStored%16]);
    Serial.print(" ");
    
    Serial.print("\n");
  }
}

void writeInstructionsToEEPROM(int chipSelect) {
  int address = 0;
  for(int i = chipSelect; i < instructionSize; i += 2) {
    writeEEPROM(address, instructions[i]);
    address++;
  }
}

void setup() {

  // setup
  pinMode(SHIFT_DATA, OUTPUT);
  pinMode(SHIFT_CLK, OUTPUT);
  pinMode(EEPROM_A12, OUTPUT);
  digitalWrite(WRITE_EN, HIGH);
  pinMode(WRITE_EN, OUTPUT);
  Serial.begin(9600);

  // write to EEPROM
  writeInstructionsToEEPROM(1);
  
  // Read and print out the contents of the EERPROM
  Serial.println("\nReading EEPROM...");

  delay(1000);
  
  printContents(0, instructionSize/2);
  Serial.println("done.\n-----------------------------------------------");
}


void loop() {
  // put your main code here, to run repeatedly:

}
