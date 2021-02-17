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

uint8_t state = STATE_NOTEBOOK;
bool changedState = false;

uint8_t dataFromIR = 0;

uint8_t irState = 0;
uint8_t bitPosition = 0;
uint8_t irDeviceId = 0;
uint8_t irDeviceIdInverted = 0;
uint8_t irData = 0;
uint8_t irDataInverted = 0;
uint8_t irCounter = 0;
uint8_t irThrowawayCounter = 0;
bool isDataValid = false;
bool isReadingActive = false;

uint32_t timeOldUs = 0;
uint32_t timeIRUs = 0;

uint32_t lastButtonPressedtimeMs = 0;
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
  //debugOutput();
  if (!isDataValid) {
    DigiKeyboard.delay(LAG);
    return;
  }
  // We have data in IR buffer (dataFromIR)  
  // DigiKeyboard.sendKeyStroke(0);
  // DigiKeyboard.delay(10);
  switch (dataFromIR) { // Assign functions to the buttons
    case PLAY_BUTTON:   press(" ");break;
    case PAUSE_BUTTON:  press(" ");break;
    case PREV_CHAPTER:  press("p");break;
    case NEXT_CHAPTER:  press("n");break;
    case STOP_BUTTON:   press("s");break;
    case FAST_BACKWARD: press("h");break;
    case FAST_FORWARD:  press("g");break;
    case SCENE_DVD:     /*printState(state);DigiKeyboard.println(" DVD ");*/changeState(COMMAND_TOSTOLOVEPC);/*printState(state);*/break;
    case SCENE_TV:      /*printState(state);DigiKeyboard.println("  TV ");*/changeState(COMMAND_TONOTEBOOK); /*printState(state);*/break;
    case SCENE_NET:     /*printState(state);DigiKeyboard.println(" NET ");*/changeState(COMMAND_TOSTOLOVEPC);/*printState(state);*/break;
    case SCENE_RADIO:   /*printState(state);DigiKeyboard.println("RADIO");*/changeState(COMMAND_TOSTOLOVEPC);/*printState(state);*/break;
    default:            break;
  }
  isDataValid = false;
  irCounter = 0;
}

void press(char* str) {
  DigiKeyboard.print(str);
  lightTheLed(300);
}

void changeState(uint8_t command) {
  updateState();
  if (command == COMMAND_TOSTOLOVEPC) {
    uint8_t newState = STATE_STOLOVEPC_AWAITDOUBLECLICK;
    switch (state) {
      case STATE_STOLOVEPC:                  break;
      case STATE_STOLOVEPC_AWAITDOUBLECLICK: newState = STATE_STOLOVEPC;pushTheButtonDouble();break;
      case STATE_NOTEBOOK:                   pushTheButtonSingle();break;
      case STATE_NOTEBOOK_AWAITDOUBLECLICK:  pushTheButtonSingle();break;
    default:                                 break;
    }
    state = newState;
  }
  else if (command == COMMAND_TONOTEBOOK) {
    uint8_t newState = STATE_NOTEBOOK_AWAITDOUBLECLICK;
    switch (state) {
      case STATE_STOLOVEPC:                  pushTheButtonSingle();break;
      case STATE_STOLOVEPC_AWAITDOUBLECLICK: pushTheButtonSingle();break;
      case STATE_NOTEBOOK:                   break;
      case STATE_NOTEBOOK_AWAITDOUBLECLICK:  newState = STATE_NOTEBOOK;pushTheButtonDouble();break;
    default:                                 break;
    }
    state = newState;
  } else {
    lightTheLed(1500);
    DigiKeyboard.delay(500);
    lightTheLed(1500);
  }
}

/* "Lazy" way of saying that after DOUBLECLICK_TIMEOUT_MS ms doubleclick is no 
 * longer available.
 */
void updateState() {
  timeNowMs = millis();
  uint32_t deltaMs = timeNowMs - lastButtonPressedtimeMs;
  if (deltaMs > DOUBLECLICK_TIMEOUT_MS) {
    if (state == STATE_NOTEBOOK_AWAITDOUBLECLICK) { state = STATE_NOTEBOOK; }
    if (state == STATE_STOLOVEPC_AWAITDOUBLECLICK) { state = STATE_STOLOVEPC; }
  }
  lastButtonPressedtimeMs = timeNowMs;
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
  lightTheLed(GATE_DELAY_MS);
  digitalWrite(GATE_PIN, LOW);
}

void pushTheButtonSingle() {
  pushTheButton();
  DigiKeyboard.delay(100);
  lightTheLed(450);
  DigiKeyboard.delay(100);
}

void pushTheButtonDouble() {
  pushTheButton();
  DigiKeyboard.delay(100);
  lightTheLed(150);
  DigiKeyboard.delay(100);
  lightTheLed(150);
  DigiKeyboard.delay(100);
}

void lightTheLed(uint16_t timeMs) {
  digitalWrite(BOARD_LED_PIN, HIGH);
  DigiKeyboard.delay(timeMs);
  digitalWrite(BOARD_LED_PIN, LOW);
}

void debugOutput() {
  lightTheLed(500);
  DigiKeyboard.print("");
  DigiKeyboard.print(uintToStr(millis()));
  DigiKeyboard.print("\t");
  DigiKeyboard.print(uintToStr(isDataValid));
  DigiKeyboard.print("\t");
  DigiKeyboard.print(uintToStr(dataFromIR));
  DigiKeyboard.print("\t");
  DigiKeyboard.print(uintToStr(state));
  DigiKeyboard.print("\t");
  DigiKeyboard.print(uintToStr(irCounter));
  DigiKeyboard.print("\t");
  DigiKeyboard.print(uintToStr(irThrowawayCounter));
  // DigiKeyboard.print("\t");
  DigiKeyboard.print("\n");
  DigiKeyboard.delay(3*LAG);
}

char* uintToStr(uint32_t value) {
  if (value == 0) {
    return "0";
  }
  static char buffer[32] = {0};
  int i = 30;
  for(; value && i ; --i, value /= 10)
  buffer[i] = "0123456789abcdef"[value % 10];
  return &buffer[i+1];
}

/* Read the IR code with NEC protocol. */
void IRNECRead(void) {
  if (isDataValid) {
    return;
  }
  timeIRUs = micros();
  uint32_t timeDeltaBig = timeIRUs - timeOldUs;
  timeOldUs = timeIRUs;
  
  if (timeDeltaBig > 14000) {
    isReadingActive = false;
    return;
  }
  
  //uint16_t timeDelta = (uint16_t) timeDeltaBig;
  uint32_t timeDelta = timeDeltaBig;
  
  if (timeDelta <= 2600) {
    if (!isReadingActive || timeDelta <= 950) {
      isReadingActive = false;
      return;
    }

    // read bit
    irCounter++;
    uint8_t bitValue = (timeDelta > 1500) ? 1 : 0;
    irData <<= 1;
    irData |= bitValue;
    
    if (irCounter == 24) {
      dataFromIR = irData;
      isReadingActive = false;
      isDataValid = true;
    }
  } else if (timeDelta <= 12100) {
    isReadingActive = false;
  } else if (timeDelta <= 14000) {
    isReadingActive = true;
    irData = 0;
    irCounter = 0;
  } 
  
}
