/*
 * Reads IR NEC signal and choses between sending signal via USB Keyboard and 
 * sending signal to configured pin (which can drive mosfet gate for example).
 * 
 * Microcontroller: ATtiny85
 * Dev board:       Digispark
 */

#include "DigiKeyboard.h"

static const uint8_t PLAY_BUTTON = 22;
static const uint8_t PAUSE_BUTTON = 230;
static const uint8_t PREV_CHAPTER = 54;
static const uint8_t NEXT_CHAPTER = 182;
static const uint8_t STOP_BUTTON = 150;
static const uint8_t FAST_BACKWARD = 86;
static const uint8_t FAST_FORWARD = 214;
static const uint8_t SCENE_DVD = 0;
static const uint8_t SCENE_TV = 192;
static const uint8_t SCENE_NET = 96;
static const uint8_t SCENE_RADIO = 144;
static const uint8_t STATE_STOLOVEPC = 10;
static const uint8_t STATE_STOLOVEPC_AWAITDOUBLECLICK = 11;
static const uint8_t STATE_NOTEBOOK = 20;
static const uint8_t STATE_NOTEBOOK_AWAITDOUBLECLICK = 21;
static const uint8_t COMMAND_TOSTOLOVEPC = 0;
static const uint8_t COMMAND_TONOTEBOOK = 1;
static const uint32_t DOUBLECLICK_TIMEOUT_MS = 3000;
static const uint8_t BOARD_LED_PIN = 1;
static const uint8_t IR_PIN = 2;
static const uint8_t GATE_PIN = 0;
static const uint8_t GATE_DELAY_MS = 1000;
static const uint16_t LAG = 333;

static const uint8_t TRANSMIT_STARTED = 1;
static const uint8_t BUTTON_DEPRESSED = 2;
static const uint8_t BIT_RECEIVED = 3;
static const uint8_t NOISE = 5;

uint8_t state = STATE_STOLOVEPC;
bool changedState = false;

uint8_t dataFromIR = 0;

uint8_t irState = 0;
uint8_t signalBit = 0;
uint8_t bitValue = 0;
uint8_t irDeviceId = 0;
uint8_t irDeviceIdInverted = 0;
uint8_t irData = 0;
uint8_t irDataInverted = 0;
bool isDataValid = false;
bool isSignalParsed = false;

uint32_t timeOldUs = 0;
uint32_t timeIRUs = 0;

uint32_t LastButtonPressedtimeMs = 0;
uint32_t timeNowMs = 0;

uint32_t interruptCounter = 0;
uint32_t interruptDiff = 0;

void setup(void) {
  noInterrupts(); //disable interrupts during setup
  //CLKPR = 0b10000000; // enable prescaler speed change
  //CLKPR = 0; // set prescaler to default (16mhz) mode required by bootloader
  pinMode(BOARD_LED_PIN, OUTPUT);
  digitalWrite(BOARD_LED_PIN, LOW);
  pinMode(GATE_PIN, OUTPUT);
  digitalWrite(GATE_PIN, LOW);
  pinMode(IR_PIN, INPUT);
  digitalWrite(IR_PIN, LOW);
  attachInterrupt(0, IRNECRead, FALLING); // Use INT0(P2) on the Digispark
  DigiKeyboard.delay(LAG);
  lightTheLed(1000);
  interrupts(); // enable interrupts as a last command of setup
}

void loop(void) {
  debugOutput();
  if (!isDataValid) {
    DigiKeyboard.delay(3*LAG);
    return;
  }
  // We have data in IR buffer (dataFromIR)  
  // DigiKeyboard.sendKeyStroke(0);
  // DigiKeyboard.delay(10);
  switch (dataFromIR) { // Assign functions to the buttons
    case PLAY_BUTTON:   DigiKeyboard.print(" ");break;
    case PAUSE_BUTTON:  DigiKeyboard.print(" ");break;
    case PREV_CHAPTER:  DigiKeyboard.print("p");break;
    case NEXT_CHAPTER:  DigiKeyboard.print("n");break;
    case STOP_BUTTON:   DigiKeyboard.print("s");break;
    case FAST_BACKWARD: DigiKeyboard.print("h");break;
    case FAST_FORWARD:  DigiKeyboard.print("g");break;
    case SCENE_DVD:     /*printState(state);DigiKeyboard.println(" DVD ");*/changeState(COMMAND_TOSTOLOVEPC);/*printState(state);*/break;
    case SCENE_TV:      /*printState(state);DigiKeyboard.println("  TV ");*/changeState(COMMAND_TONOTEBOOK); /*printState(state);*/break;
    case SCENE_NET:     /*printState(state);DigiKeyboard.println(" NET ");*/changeState(COMMAND_TOSTOLOVEPC);/*printState(state);*/break;
    case SCENE_RADIO:   /*printState(state);DigiKeyboard.println("RADIO");*/changeState(COMMAND_TOSTOLOVEPC);/*printState(state);*/break;
    default:            break;
  }
  isDataValid = false;
}

void changeState(uint8_t command) {
  updateState();
  if (command == COMMAND_TOSTOLOVEPC) {
    uint8_t newState = STATE_STOLOVEPC_AWAITDOUBLECLICK;
    switch (state) {
    case STATE_STOLOVEPC:                  break;
    case STATE_STOLOVEPC_AWAITDOUBLECLICK: newState = STATE_STOLOVEPC; pushTheButton();break;
    case STATE_NOTEBOOK:                   pushTheButton();break;
    case STATE_NOTEBOOK_AWAITDOUBLECLICK:  pushTheButton();break;
    default:                               break;
    }
    state = newState;
  }
  else if (command == COMMAND_TONOTEBOOK) {
    uint8_t newState = STATE_NOTEBOOK_AWAITDOUBLECLICK;
    switch (state) {
      case STATE_STOLOVEPC:                  pushTheButton();break;
      case STATE_STOLOVEPC_AWAITDOUBLECLICK: pushTheButton();break;
      case STATE_NOTEBOOK:                   break;
      case STATE_NOTEBOOK_AWAITDOUBLECLICK:  newState = STATE_NOTEBOOK;pushTheButton();break;
    default:                                 break;
    }
    state = newState;
  } else {
    lightTheLed(1500);
  }
}

/* "Lazy" way of saying that after DOUBLECLICK_TIMEOUT_MS ms doubleclick is no 
 * longer available.
 */
void updateState() {
  timeNowMs = millis();
  uint32_t deltaMs = timeNowMs - LastButtonPressedtimeMs;
  if (deltaMs > DOUBLECLICK_TIMEOUT_MS) {
    if (state == STATE_NOTEBOOK_AWAITDOUBLECLICK) { state = STATE_NOTEBOOK; }
    if (state == STATE_STOLOVEPC_AWAITDOUBLECLICK) { state = STATE_STOLOVEPC; }
  }
  LastButtonPressedtimeMs = timeNowMs;
  // printState(state);
}

void printState(uint8_t state) {
  switch (state) {
    case STATE_STOLOVEPC:                  DigiKeyboard.println("_PC_");break;
    case STATE_STOLOVEPC_AWAITDOUBLECLICK: DigiKeyboard.println("_PC DBL_");break;
    case STATE_NOTEBOOK:                   DigiKeyboard.println("_NTB_");break;
    case STATE_NOTEBOOK_AWAITDOUBLECLICK:  DigiKeyboard.println("_NTB DBL_");break;
    default:                               break;
    }
}

void pushTheButton() {
  digitalWrite(GATE_PIN, HIGH);
  DigiKeyboard.delay(GATE_DELAY_MS);
  digitalWrite(GATE_PIN, LOW);
}

void lightTheLed(uint16_t timeMs) {
  digitalWrite(BOARD_LED_PIN, HIGH);
  DigiKeyboard.delay(timeMs);
  digitalWrite(BOARD_LED_PIN, LOW);
}

void debugOutput() {
  lightTheLed(500);
  DigiKeyboard.print("");
  printInt(millis());
  DigiKeyboard.print("\t");
  printInt(isDataValid);
  DigiKeyboard.print("\t");
  printInt(dataFromIR);
  DigiKeyboard.print("\t");
  printInt(state);
  // DigiKeyboard.print("\t");
  DigiKeyboard.print("\n");
}

/* Read the IR code with NEC protocol. */
void IRNECRead(void) {
  if (isDataValid) {
    return;
  }
  timeIRUs = micros();
  uint32_t timeDelta = timeIRUs - timeOldUs;
  timeOldUs = timeIRUs;
  if (timeDelta <= 950) {
    isSignalParsed = false;
    signalBit = 0;
  } else if (timeDelta <= 1500) {
    // 1125 +- 158 -> 967, 1283, 1441
    irState = BIT_RECEIVED; // read bit "0"
    bitValue = 0;
  } else if (timeDelta <= 2600) {
    // 2250 +- 158 -> 2097, 2408, 2564
    irState = BIT_RECEIVED; // read bit "1"
    bitValue = 1;
  } else if (timeDelta <= 10000) {
    isSignalParsed = false;
    signalBit = 0;
  } else if (timeDelta <= 12100) {
    // 11250
    irState = BUTTON_DEPRESSED; // signal repeated
  } else if (timeDelta <= 14000) {
    // 13000
    isSignalParsed = false;
    signalBit = 0;
    irState = TRANSMIT_STARTED;
    irDeviceId = 0;
    irDeviceIdInverted = 0;
    irData = 0;
    irDataInverted = 0;
  } else {
    isSignalParsed = false;
    signalBit = 0;
  }    

  if (irState == BIT_RECEIVED) {
    if (signalBit < 8) {
      irDeviceId |= bitValue;
      if (signalBit < 7) irDeviceId <<= 1;
      signalBit++;
    } else if (signalBit < 16) {
      irDeviceIdInverted |= bitValue;
      if (signalBit < 15) irDeviceIdInverted <<= 1;
      signalBit++;
    } else if (signalBit < 24) {
      irData |= bitValue;
      if (signalBit < 23) irData <<= 1;
      signalBit++;
    } else if (signalBit < 32) {
      irDataInverted |= bitValue;
      if (signalBit < 31) {
        irDataInverted <<= 1;
      } else {
        dataFromIR = irData;
        isSignalParsed = true;
        isDataValid = true;
      }
      signalBit++;
    }
  } else if (irState == BUTTON_DEPRESSED) {
    if (isSignalParsed) {
      isDataValid = true;
    }
  }
}

void printInt(uint32_t value) {
  char buffer[20]; //this array will hold the ASCII codes generated by the function

  itoa(value, buffer, DEC);

  DigiKeyboard.print(buffer);
}
