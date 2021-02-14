/*************************************
* Send received IR signal via SerialUSB.
* Read it with: cu -l /dev/ttyACM0 -s 19200
* 
* Microcontroller: ATtiny85
* Dev board:       Digispark
* 
*************************************/
#include <Arduino.h>
#include <DigiCDC.h>
#include "IR_Read.h"

void setup(void) {
  attachInterrupt(0, IR_Read, FALLING);
  
  pinMode(1,OUTPUT);
  digitalWrite(1,LOW);

  SerialUSB.begin();
}
 
void loop(void) {
  digitalWrite(1,HIGH);
    SerialUSB.delay(500);
  digitalWrite(1,LOW);
  if(Data_valid == 1) {
    SerialUSB.println(Data_back, DEC);
    Data_valid = 0;
  } else {
    SerialUSB.delay(4500);
  }
}

