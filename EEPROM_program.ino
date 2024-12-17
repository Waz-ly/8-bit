/**
 * This sketch programs the microcode EEPROMs for the 8-bit breadboard computer
 * It includes support for a flags register with carry and zero flags
 * See this video for more: https://youtu.be/Zg1NdPKoosU
 */

/*
 * Command = larger instruction, ie PUSH, PULL, MOV, etc
 * 
 * Instruction = smaller instruction, ie the command MOV <A, B> is split into instructions A out, B in
 */

#define li long long int

#define SHIFT_DATA 2
#define SHIFT_CLK 3
#define BIG_CHIP_WRITE_EN 4
#define EEPROM_D0 5
#define EEPROM_D7 12
#define WRITE_EN 13

char hexmap[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

  int chipSelect = 1;

  li none                     = 0b000000000000000000000000;
  li invert                   = 0b000011100000101000011010;
  li andWithClock             = 0b000000001000010001000101;

  li aRegisterIn              = 0b000000000000000000000001;
  li aRegisterOut             = 0b000000000000000000000010;
  li bRegisterIn              = 0b000000000000000000000100;
  li bRegisterOut             = 0b000000000000000000001000;
  li summationOut             = 0b000000000000000000010000;
  li setSubtraction           = 0b000000000000000000100000;
  li setFlags                 = 0b000000000000000001000000;

  li ramDisableOut            = 0b000000000000000100000000;
  li ramIn                    = 0b000000000000001000000000;
  li ramAddressIn             = 0b000000000000010000000000;
  li busExchangeActive        = 0b000000000000100000000000;
  li out                      = 0b000000001000000000000000;
  
  li incrementProgramCounter  = 0b000000010000000000000000;
  li upperProgramCounterIn    = 0b000000100000000000000000;
  li lowerProgramCounterIn    = 0b000001000000000000000000;
  li eepromDataOut            = 0b000010000000000000000000;
  li latchInstructions        = 0b000100000000000000000000;
  li resetCounter             = 0b001000000000000000000000;


  li moveAtoB[4]        = { aRegisterOut|bRegisterIn                          , 0                                                       , incrementProgramCounter                 , latchInstructions };
  li moveAtoRAM[4]      = { eepromDataOut|ramAddressIn                        , aRegisterOut|ramIn|ramDisableOut|busExchangeActive      , incrementProgramCounter                 , latchInstructions };
  li moveBtoA[4]        = { bRegisterOut|aRegisterIn                          , 0                                                       , incrementProgramCounter                 , latchInstructions };
  li moveBtoRAM[4]      = { eepromDataOut|ramAddressIn                        , bRegisterOut|ramIn|ramDisableOut|busExchangeActive      , incrementProgramCounter                 , latchInstructions };
  li moveRAMtoA[4]      = { eepromDataOut|ramAddressIn                        , aRegisterIn|busExchangeActive                           , incrementProgramCounter                 , latchInstructions };
  li moveRAMtoB[4]      = { eepromDataOut|ramAddressIn                        , bRegisterIn|busExchangeActive                           , incrementProgramCounter                 , latchInstructions };
  li loadA[4]           = { eepromDataOut|aRegisterIn                         , 0                                                       , incrementProgramCounter                 , latchInstructions };
  li loadB[4]           = { eepromDataOut|bRegisterIn                         , 0                                                       , incrementProgramCounter                 , latchInstructions };
  li sumToA[4]          = { setFlags                                          , summationOut|aRegisterIn                                , incrementProgramCounter                 , latchInstructions };
  li sumToB[4]          = { setFlags                                          , summationOut|bRegisterIn                                , incrementProgramCounter                 , latchInstructions };
  li subToA[4]          = { setSubtraction|setFlags                           , setSubtraction|summationOut|aRegisterIn                 , incrementProgramCounter                 , latchInstructions };
  li subToB[4]          = { setSubtraction|setFlags                           , setSubtraction|summationOut|bRegisterIn                 , incrementProgramCounter                 , latchInstructions };
  li jump[4]            = { aRegisterOut|lowerProgramCounterIn                , bRegisterOut|upperProgramCounterIn                      , 0                                       , latchInstructions };
  li jumpIfZero[4]      = { aRegisterOut|lowerProgramCounterIn                , bRegisterOut|upperProgramCounterIn                      , 0                                       , latchInstructions };
  li jumpIfCarry[4]     = { aRegisterOut|lowerProgramCounterIn                , bRegisterOut|upperProgramCounterIn                      , 0                                       , latchInstructions };
  li Aout[4]            = { aRegisterOut                                      , aRegisterOut|out                                        , incrementProgramCounter                 , latchInstructions };
  li nop[4]             = { none                                              , none                                                    , incrementProgramCounter                 , latchInstructions };

  li *commands[32] = { moveAtoB, moveAtoRAM, moveBtoA, moveBtoRAM, moveRAMtoA, moveRAMtoB, loadA, loadB, sumToA, sumToB, subToA, subToB, nop, nop, jump, jumpIfZero,
    jumpIfCarry, Aout, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop, nop };

void invertFunction(li **arr) {
  for(int i = 0; i < 32; i++) {
    for(int j = 0; j < 4; j++) {
      arr[i][j] ^= invert;
    }
  }

  for(int i = 0; i < 4; i++) {
    arr[31][i] ^= invert;
  }
}

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

  int clk, flags, command, instruction;
  for (int address = 0; address < 1024; address++) {
    clk = (address >> 9)%2;
    flags = (address >> 7)%4;
    command = (address >> 2)%32;
    instruction = address%4;

    int writeToChip = commands[command][instruction] >> (chipSelect*8);

    if(command == 15) {
      if((flags)%2 == 0) {
        writeToChip = commands[19][instruction] >> (chipSelect*8);
      }
    } else if(command == 16) {
      if((flags >> 1)%2 == 0) {
        writeToChip = commands[19][instruction] >> (chipSelect*8);
      }
    }

    writeToChip = (writeToChip & (clk*(andWithClock >> (chipSelect*8)))) | (writeToChip & ~(andWithClock >> (chipSelect*8)));

    writeEEPROM(address, writeToChip);
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

  invertFunction(commands);

  // write to EEPROM
  writeCodeToEEPROM();
  
  // Read and print out the contents of the EERPROM
  Serial.println("\nReading EEPROM...");
  printContents(0, 1024);
  Serial.println("done.\n-----------------------------------------------");
}


void loop() {
  // put your main code here, to run repeatedly:

}
