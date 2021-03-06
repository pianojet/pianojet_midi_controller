#include <Arduino.h>
#include <Key.h>
#include <Keypad.h>
#include <Adafruit_NeoTrellis.h>
#include <Adafruit_NeoPixel.h>

#include <ClickEncoder.h>
#include <TimerOne.h>
#include <MIDI.h>

#include "/Users/justintaylor/Documents/Arduino/pianojet/colors.h"

#define POTPIN 14  //wiper
#define RIBBON_LED_PIN 22
#define LOGGING true

#define Y_TRELLIS 4
#define X_TRELLIS 8

//
// definitions & types //////////////////////////////////////////////////////////
//

//
// Trellis
//

// create a matrix of trellis panels
Adafruit_NeoTrellis t_array[Y_TRELLIS/4][X_TRELLIS/4] = {
  
  { Adafruit_NeoTrellis(0x2E), Adafruit_NeoTrellis(0x2F) }
  
};
Adafruit_MultiTrellis trellis((Adafruit_NeoTrellis *)t_array, Y_TRELLIS/4, X_TRELLIS/4);
const byte TRELLIS_MODE_COUNT = 8;
const byte TRELLIS_STANDBY = 0;
const byte TRELLIS_TOGGLE = 1;
const byte TRELLIS_LOOP_OFF = 2;
const byte TRELLIS_LOOP_CUED = 3;
const byte TRELLIS_TRIGGER_OFF = 4;
const byte TRELLIS_TRIGGER_CUED = 5;
const byte TRELLIS_PRESSED_ON = 6;
const byte TRELLIS_PRESSED_OFF = 7;

uint32_t colorMap[TRELLIS_MODE_COUNT];
uint8_t trellisModeMatrix[Y_TRELLIS][X_TRELLIS];

//
// Ribbon
//
const byte DEFAULT_RIBBON_VELOCITY = 100;
const byte DEFAULT_RIBBON_NOOP = 0;

struct RibbonControllerSettings {
    uint16_t 
        lastWiperState,
        analogOffset;

    uint8_t
        lastWiperNote,
        windowRadius,
        analogCutoff,
        analogScaleFactor,
        
        velocityLeft,
        velocityRight,

        channelLeft,
        channelRight,

        customLeft,
        customRight;

    bool
        shouldIPlay,
        pitchBendMode;
};

//
// Keypad
//

const byte KEYPAD_ROWS = 4;
const byte KEYPAD_COLS = 3;
const byte KEYPAD_KEYSTROKES = 5;
struct KeypadControllerSettings {
    uint16_t inputNumber;
    uint8_t inputMenu;
    char
        keys[KEYPAD_ROWS][KEYPAD_COLS],
        inputString[KEYPAD_KEYSTROKES];

    byte 
        rowPins[KEYPAD_ROWS],
        colPins[KEYPAD_COLS];
};

//
// rotary encoders
//

const byte ROTARY_COUNT = 4;

// RotaryControllerAdapter
class RotaryControllerAdapter {

public:
    RotaryControllerAdapter(void (*buttonFn)(ClickEncoder::Button), void (*valueFn)(int16_t), int16_t defaultValue);
    uint8_t id;
    int16_t defaultValue;

    void processButton(ClickEncoder::Button button);
    void processValue(int16_t value);
    void setButtonFn(void (*buttonFn)(ClickEncoder::Button));
    void setValueFn(void (*valueFn)(int16_t));

    ~RotaryControllerAdapter();
private:
    void (*_processButton)(RotaryControllerAdapter*, ClickEncoder::Button);
    void (*_processValue)(RotaryControllerAdapter*, int16_t);
};

RotaryControllerAdapter::RotaryControllerAdapter(void (*buttonFn)(ClickEncoder::Button), void (*valueFn)(int16_t), int16_t defaultValue) {
    this->defaultValue = defaultValue;
    this->_processButton = buttonFn;
    this->_processValue = valueFn;
}
void RotaryControllerAdapter::setButtonFn(void (*buttonFn)(ClickEncoder::Button)) {
    this->_processButton = buttonFn;
}
void RotaryControllerAdapter::setValueFn(void (*valueFn)(int16_t)) {
    this->_processValue = valueFn;
}
void RotaryControllerAdapter::processButton(ClickEncoder::Button button) {
    this->_processButton(this, button);
}
void RotaryControllerAdapter::processValue(int16_t value) {
    this->_processValue(this, value);
}
RotaryControllerAdapter::~RotaryControllerAdapter() {
}


// RotaryController
class RotaryController {
public:
    RotaryController(uint8_t, uint8_t, uint8_t, RotaryControllerAdapter*);
    uint8_t
        id,
        currentValue,
        lastValue;
    ClickEncoder::Button
        currentButton,
        lastButton;
    RotaryControllerAdapter * adapter;
    ClickEncoder *encoder;
    void tick();
    ~RotaryController();
};
RotaryController::RotaryController(uint8_t rotaryPin01, uint8_t rotaryPin02, uint8_t buttonPin, RotaryControllerAdapter * adapter) {
    this->id = rotaryPin01;
    this->encoder = new ClickEncoder(rotaryPin01, rotaryPin02, buttonPin);
    this->adapter = adapter;
    this->adapter->id = this->id;
    this->currentButton = NULL;
    this->lastButton = NULL;
}
void RotaryController::tick() {
    this->currentValue = this->encoder->getValue();
    if (this->currentValue != this->lastValue) {
        this->adapter->processValue(this->currentValue);
        this->lastValue = this->currentValue;
    }

    this->currentButton = this->encoder->getButton();
    if (this->currentButton != ClickEncoder::Open) {
        this->adapter->processButton(this->currentButton);
        this->lastButton = this->currentButton;
    }
}
RotaryController::~RotaryController() {
    delete this->adapter;
    delete this->encoder;
}

//
// declarations & initializations //////////////////////////////////////////////////////////
//

RibbonControllerSettings rSettings;

KeypadControllerSettings kSettings;
Keypad * keypad = NULL;

void ribbonNOOPButton(RotaryControllerAdapter self, ClickEncoder::Button button) {
    logger("Rotary ");logger(self.id);logger(" (NOOP) button: ");
    switch (button) {
        case ClickEncoder::Pressed:
            logger("  Pressed\n");
        break;

        case ClickEncoder::Held:
            logger("  Held\n");
        break;

        case ClickEncoder::Released:
            logger("  Released\n");
        break;

        case ClickEncoder::Clicked:
            logger("  Clicked\n");
        break;

        case ClickEncoder::DoubleClicked:
            logger("  DoubleClicked\n");
        break;
    }
}
void ribbonNOOPValue(RotaryControllerAdapter self, int16_t value) {
    logger("Rotary ");logger(self.id);logger(" (NOOP) value: ");logger(value); logger("\n");
}

void ribbonVelocityButton(RotaryControllerAdapter self, ClickEncoder::Button button) {
    logger("Rotary ");logger(self.id);logger(" button: ");
    switch (button) {
        case ClickEncoder::Pressed:
            logger("  Pressed\n");
        break;

        case ClickEncoder::Held:
            logger("  Held\n");
        break;

        case ClickEncoder::Released:
            logger("  Released\n");
        break;

        case ClickEncoder::Clicked:
            logger("  Clicked, setting ribbon velocity to default\n");

            rSettings.velocityLeft = self.defaultValue;
            rSettings.velocityRight = self.defaultValue;

            logger("Velocity 01 new value: "); logger(rSettings.velocityLeft); logger("\n");
            logger("Velocity 02 new value: "); logger(rSettings.velocityRight); logger("\n");
        break;

        case ClickEncoder::DoubleClicked:
            logger("  DoubleClicked\n");
        break;
    }
}
void ribbonVelocityValue(RotaryControllerAdapter self, int16_t value) {
    logger("Rotary ");logger(self.id);logger(" value: ");logger(value); logger("\n");

    rSettings.velocityLeft += value;
    rSettings.velocityRight += value;

    if (rSettings.velocityLeft < 0) { rSettings.velocityLeft = 0;}
    if (rSettings.velocityLeft > 127) { rSettings.velocityLeft = 127;}

    if (rSettings.velocityRight < 0) { rSettings.velocityRight = 0;}
    if (rSettings.velocityRight > 127) { rSettings.velocityRight = 127;}

    logger("Velocity 01 new value: "); logger(rSettings.velocityLeft); logger("\n");
    logger("Velocity 02 new value: "); logger(rSettings.velocityRight); logger("\n");
}
RotaryControllerAdapter *ribbonAdapterRibbonVelocity = NULL;
RotaryControllerAdapter *ribbonAdapterNOOP01 = NULL;
RotaryControllerAdapter *ribbonAdapterNOOP02 = NULL;
RotaryControllerAdapter *ribbonAdapterNOOP03 = NULL;
RotaryController *rotary[ROTARY_COUNT] = {NULL, NULL, NULL, NULL};




#ifndef LOGGING
MIDI_CREATE_DEFAULT_INSTANCE();
#endif

void pianojetInit() {
    // trellis
    colorMap[TRELLIS_STANDBY] = LIGHTGREEN;
    colorMap[TRELLIS_TOGGLE] = DARKGREEN;
    colorMap[TRELLIS_LOOP_OFF] = BLUE;
    colorMap[TRELLIS_LOOP_CUED] = PURPLE;
    colorMap[TRELLIS_TRIGGER_OFF] = RED;
    colorMap[TRELLIS_TRIGGER_CUED] = ORANGE;
    colorMap[TRELLIS_PRESSED_ON] = YELLOW;
    colorMap[TRELLIS_PRESSED_OFF] = LIGHTGREEN;

    for(int y=0; y<Y_TRELLIS; y++) {
        for(int x=0; x<X_TRELLIS; x++) {
            trellisModeMatrix[x][y] = TRELLIS_STANDBY;
        }
    }

    // ribbon settings
    resetRibbonStateNoteMode();

    // keypad settings
    kSettings.keys[0][0] = '1';
    kSettings.keys[0][1] = '2';
    kSettings.keys[0][2] = '3';

    kSettings.keys[1][0] = '4';
    kSettings.keys[1][1] = '5';
    kSettings.keys[1][2] = '6';

    kSettings.keys[2][0] = '7';
    kSettings.keys[2][1] = '8';
    kSettings.keys[2][2] = '9';

    kSettings.keys[3][0] = '*';
    kSettings.keys[3][1] = '0';
    kSettings.keys[3][2] = '#';

    kSettings.rowPins[0] = 2;
    kSettings.rowPins[1] = 3;
    kSettings.rowPins[2] = 4;
    kSettings.rowPins[3] = 5;

    kSettings.rowPins[4] = 6;
    kSettings.rowPins[5] = 7;
    kSettings.rowPins[6] = 8;

    resetKeypadState();
    if (keypad) {
        delete keypad;
    }
    keypad = new Keypad( makeKeymap(kSettings.keys), kSettings.rowPins, kSettings.colPins, KEYPAD_ROWS, KEYPAD_COLS );

    // rotary settings
    for (int r = 0; r < ROTARY_COUNT; r++) {
        if (rotary[r]) {
            delete rotary[r];
        }
    }
    ribbonAdapterRibbonVelocity = new RotaryControllerAdapter(ribbonVelocityButton, ribbonVelocityValue, DEFAULT_RIBBON_VELOCITY);
    ribbonAdapterNOOP01 = new RotaryControllerAdapter(ribbonNOOPButton, ribbonNOOPValue, DEFAULT_RIBBON_NOOP);
    ribbonAdapterNOOP02 = new RotaryControllerAdapter(ribbonNOOPButton, ribbonNOOPValue, DEFAULT_RIBBON_NOOP);
    ribbonAdapterNOOP03 = new RotaryControllerAdapter(ribbonNOOPButton, ribbonNOOPValue, DEFAULT_RIBBON_NOOP);
    rotary[0] = new RotaryController(A0, A1, A2, ribbonAdapterRibbonVelocity);
    rotary[1] = new RotaryController(A3, A4, A5, ribbonAdapterNOOP01);
    rotary[2] = new RotaryController(A8, A9, A10, ribbonAdapterNOOP02);
    rotary[3] = new RotaryController(A11, A12, A13, ribbonAdapterNOOP03);
}

//
// arduino setup & loop  //////////////////////////////////////////////////////////
//


int16_t last, value;
void timerIsr() {
    for (int r = 0; r < ROTARY_COUNT; r++) {
        if (rotary[r]) {
            rotary[r]->encoder->service();
        }
    }
}



void setup()
{
    Serial.begin(9600);
    Serial.println("STARTING!\n");
 
    pianojetInit();
 
    midiBegin();

    allOff();

    logger(rSettings);

    if(!trellis.begin()){
        logger("failed to begin trellis");
        while(1);
    } else {
        logger("Trellis online.\n");
    }

    // the array can be addressed as x,y or with the key number */
    for(int i=0; i<Y_TRELLIS*X_TRELLIS; i++){
        trellis.setPixelColor(i, Wheel(map(i, 0, X_TRELLIS*Y_TRELLIS, 0, 255))); //addressed with keynum
        trellis.show();
        delay(50);
    }

    for(int y=0; y<Y_TRELLIS; y++){
        for(int x=0; x<X_TRELLIS; x++){
            //activate rising and falling edges on all keys
            trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_RISING, true);
            trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_FALLING, true);
            trellis.registerCallback(x, y, trellisKeypress);


            // trellis.setPixelColor(x, y, getColor(x, y)); //addressed with x,y

            trellis.setPixelColor(x, y, 0x000000); //addressed with x,y

            trellis.show(); //show all LEDs
            delay(50);
        }
    }

    Timer1.initialize(1000);
    Timer1.attachInterrupt(timerIsr);
    last = -1;

}

void loop()
{
    //
    // trellis
    //

    trellis.read();

    //
    // keypad
    //

    char key = keypad->getKey();
    if (key) {
        processKey(key);
    }

    //
    // rotary
    //

    for (int r = 0; r < ROTARY_COUNT; r++) {
        if (rotary[r]) {
            rotary[r]->tick();
        }
    }

    //
    // ribbon
    //

    int reading = readWiper();
    playWiper(reading);
}

//
// util  /////////////////////////////////////////////////////////////////////
//

void logger(const char * s) {
    #ifdef LOGGING
    Serial.print(s);
    #endif
}

void logger(const char s) {
    #ifdef LOGGING
    Serial.print(s);
    #endif
}

void logger(const bool b) {
    #ifdef LOGGING
    Serial.print(b);
    #endif
}

void logger(const double s) {
    #ifdef LOGGING
    Serial.print(s);
    #endif
}

void logger(const uint8_t n) {
    #ifdef LOGGING
    Serial.print(n);
    #endif
}

void logger(const int16_t n) {
    #ifdef LOGGING
    Serial.print(n);
    #endif
}

void logger(const unsigned long n) {
    #ifdef LOGGING
    Serial.print(n);
    #endif
}

void logger(const unsigned int n) {
    #ifdef LOGGING
    Serial.print(n);
    #endif
}

void logger(const RibbonControllerSettings n) {
    #ifdef LOGGING
    Serial.println("\nRibbonControllerSettings:");
    Serial.print("\tlastWiperState:    "); Serial.println(rSettings.lastWiperState);
    Serial.print("\tanalogOffset:      "); Serial.println(rSettings.analogOffset);
    Serial.print("\tlastWiperNote:     "); Serial.println(rSettings.lastWiperNote);
    Serial.print("\twindowRadius:      "); Serial.println(rSettings.windowRadius);
    Serial.print("\tanalogCutoff:      "); Serial.println(rSettings.analogCutoff);
    Serial.print("\tanalogScaleFactor: "); Serial.println(rSettings.analogScaleFactor);
    Serial.print("\tshouldIPlay:       "); Serial.println(rSettings.shouldIPlay);
    #endif
}

void logger(const KeypadControllerSettings n) {
    #ifdef LOGGING
    Serial.println("\nKeypadControllerSettings:");
    Serial.print("\tinputString:    "); Serial.println(kSettings.inputString);
    Serial.print("\tinputMenu:      "); Serial.println(kSettings.inputMenu);
    Serial.print("\tinputNumber:    "); Serial.println(kSettings.inputNumber);
    #endif
}



//
// trellis service
//

// not really sure why, but the x and y need to be reversed below when used as index
uint32_t getColor(uint8_t x, uint8_t y) {
    return colorMap[trellisModeMatrix[y][x]];
}

uint8_t getTrellisMode(uint8_t x, uint8_t y) {
    return trellisModeMatrix[y][x];
}

void setTrellisMode(uint8_t x, uint8_t y, uint8_t mode) {
    trellisModeMatrix[y][x] = mode;
}

void trellisKeypress(keyEvent evt) {
    uint8_t x = evt.bit.NUM % (X_TRELLIS);
    uint8_t y = evt.bit.NUM / (X_TRELLIS);

    uint8_t currentMode = getTrellisMode(x, y);

    logger("Trellis, evt.bit.NUM:"); logger(evt.bit.NUM); logger(" x:"); logger(x); logger(" y:"); logger(y);
    logger(" mode:"); logger(currentMode); logger("\n");
    logger("\tactivating: ");

    switch (currentMode)
    {
        case (TRELLIS_STANDBY):
            if(evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {
                logger(" RISING, TRELLIS_TOGGLE");
                setTrellisMode(x, y, TRELLIS_TOGGLE);

            }

            else if(evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING) {
                logger(" FALLING, TRELLIS_STANDBY");
                setTrellisMode(x, y, TRELLIS_STANDBY);

            }

            break;
        case (TRELLIS_TOGGLE):
            if(evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {
                logger(" RISING, TRELLIS_TOGGLE");
                setTrellisMode(x, y, TRELLIS_TOGGLE);

            }

            else if(evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING) {
                logger(" FALLING, TRELLIS_STANDBY");
                setTrellisMode(x, y, TRELLIS_STANDBY);

            }

            break;            
        default:
            if(evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING)
                logger(" DEFAULT PRESSED");
            else
                logger(" DEFAULT RELEASED");
            break;
    }

    trellis.setPixelColor(evt.bit.NUM, getColor(x, y));

    logger("\n");

    trellis.show();
    return 0;
}
//
// ribbon service    //////////////////////////////////////////////////////////
//

void resetRibbonStateNoteMode() {
    rSettings.lastWiperState = 1023;
    rSettings.analogOffset = 1000;
    rSettings.lastWiperNote = 0;
    rSettings.windowRadius = 10;
    rSettings.analogCutoff = 20;
    rSettings.analogScaleFactor = 10;
    rSettings.shouldIPlay = false;

    rSettings.velocityLeft = DEFAULT_RIBBON_VELOCITY;
    rSettings.velocityRight = DEFAULT_RIBBON_VELOCITY;
    rSettings.channelLeft = 0;
    rSettings.channelRight = 1;
    rSettings.customLeft = 0;
    rSettings.customRight = 0;

    rSettings.pitchBendMode = false;
}

void resetRibbonStatePitchBendMode() {
    rSettings.lastWiperState = 1023;
    rSettings.analogOffset = 1000;
    rSettings.lastWiperNote = 0;
    rSettings.windowRadius = 10;
    rSettings.analogCutoff = 20;
    rSettings.analogScaleFactor = 10;
    rSettings.shouldIPlay = false;

    rSettings.velocityLeft = DEFAULT_RIBBON_VELOCITY;
    rSettings.velocityRight = DEFAULT_RIBBON_VELOCITY;
    rSettings.channelLeft = 0;
    rSettings.channelRight = 1;
    rSettings.customLeft = 0;
    rSettings.customRight = 0;

    rSettings.pitchBendMode = true;
}

int readWiper() {
    return analogRead(POTPIN);
}

byte scaleReadingNote(unsigned int reading) {
    // return (int)(reading * 0.125); // 128 / 1016
    uint8_t returnLow = 0 + rSettings.analogCutoff;
    uint8_t returnHigh = 127 - rSettings.analogCutoff;
    return map(reading, 0, 1015, returnLow, returnHigh);
}

byte scaleReadingPitchBend(unsigned int reading) {
    return map(reading, 0, 1015, 0, 127);
}

void playWiper(unsigned int reading) {

    // pitchbend mode
    if (rSettings.pitchBendMode) {

        byte localNote = scaleReadingPitchBend(reading);

        // if (reading > rSettings.analogCutoff) {

        //     if (localNote != rSettings.lastWiperNote) {
        //         noteOff(rSettings.lastWiperNote);
        //         noteOn(localNote);
        //         rSettings.shouldIPlay = false;

        //         rSettings.lastWiperNote = localNote;
        //     } 

        //     rSettings.lastWiperState = reading;

        // } else {

        //     if (rSettings.shouldIPlay == false) {
        //         noteOff(rSettings.lastWiperNote);
        //         rSettings.shouldIPlay = true;
        //     }
        // }

    // note mode
    } else {

        byte localNote = scaleReadingNote(reading);

        if (reading > rSettings.analogCutoff) {

            if (localNote != rSettings.lastWiperNote) {
                noteOff(rSettings.lastWiperNote, rSettings.velocityLeft, rSettings.channelLeft);
                noteOff(rSettings.lastWiperNote, rSettings.velocityRight, rSettings.channelRight);
                noteOn(localNote, rSettings.velocityLeft, rSettings.channelLeft);
                noteOn(localNote, rSettings.velocityRight, rSettings.channelRight);
                rSettings.shouldIPlay = false;

                rSettings.lastWiperNote = localNote;
            } 

            rSettings.lastWiperState = reading;

        } else {

            if (rSettings.shouldIPlay == false) {
                noteOff(rSettings.lastWiperNote, rSettings.velocityLeft, rSettings.channelLeft);
                noteOff(rSettings.lastWiperNote, rSettings.velocityRight, rSettings.channelRight);
                rSettings.shouldIPlay = true;
            }
        }
    }
}

bool inWindow(unsigned int reading) {
    return (abs(rSettings.lastWiperState - reading) > rSettings.windowRadius);
}

//
// keypad service    //////////////////////////////////////////////////////////
//

void resetKeypadState() {
    kSettings.inputString[0] = '-';
    kSettings.inputString[1] = '-';
    kSettings.inputString[2] = '-';
    kSettings.inputString[3] = '-';
    kSettings.inputString[4] = '-';
    kSettings.inputNumber = 0;
    kSettings.inputMenu = 0;
}

uint32_t addZeroDigit(uint16_t number) {
    long added;
    char str1[16] = "";
    char str2[16] = "";

    // could do a recursion trick here

    // sprintf(str, "%d", (int)number);
    sprintf(str1, "%d0", number);
    logger("  str1: ");
    logger(str1);


    // added = (long)strtol(str1, NULL, 10);
    // logger("  added: ");
    // logger(added);
    // if (number < 10) {
    //     char str[2] = "00";
    //     sprintf(str, "%d0", number);
    //     added = (uint8_t)(strtol(str, NULL, 0));
    // } else if (number < 100) {
    //     char str[3] = "000";
    //     sprintf(str, "%d0", number);
    //     added = (uint8_t)(strtol(str, NULL, 0));
    // }

    

    return (uint32_t)strtol(str1, NULL, 10);
}

void processKey(char key) {
    uint8_t commandPlace = 0;

    char * rearSubstr = strchr(kSettings.inputString, '-');
    commandPlace = rearSubstr - (char *)&kSettings.inputString;
    kSettings.inputString[commandPlace] = key;

    logger("Keypad: ");
    logger(key);

    logger("  commandPlace: ");
    logger(commandPlace);

    switch (key)
    {
        case '*':
            logger("  MENU\n");

            kSettings.inputMenu++;
            break;

        case '#':
            logger("  FULL RESET\n");
            return resetKeypadState();
            break;

        default:
            logger("  INPUT NUMBER UPDATE: ");
            if (key == '0' && kSettings.inputNumber == 0) {
                logger(" 0 NULL\n");
                return;
            }

            if (kSettings.inputNumber > 0) {
                kSettings.inputNumber = addZeroDigit(kSettings.inputNumber);
            }
            
            kSettings.inputNumber = kSettings.inputNumber + (key - '0');

            logger(kSettings.inputNumber);
            break;
    }

    if (commandPlace >= (KEYPAD_KEYSTROKES - 1)) {
        logger("  FULL STRING RESET\n");
        return resetKeypadState();
    }

    if (kSettings.inputMenu == 0) {
        // do something on individual number press
        switch (kSettings.inputNumber)
        {
            case 0:
                if (rSettings.pitchBendMode) { resetRibbonStateNoteMode(); } else { resetRibbonStatePitchBendMode(); }
                logger("Toggling ribbon pitchBendMode to: "); logger(rSettings.pitchBendMode); logger("\n");
            break;

            case 1:
            break;

            default:
                break;
        }

    }


    logger(kSettings);
    logger("\n");

}

//
// rotary service    //////////////////////////////////////////////////////////
//

//
// midi services
//

void handleNoteOn(byte channel, byte note, byte velocity) {

    // uint32_t color;
    // switch (channel) {
    //     case 0:
    //     case 1:
    //     break;
    //     case 2:
    //     break;
    //     case 3:
    //     case 4:
    //     case 5:
    //     case 6:
    //     case 7:
    //     case 8:
    //     case 9:
    //     case 10:
    //     case 11:
    //     case 12:
    //     case 13:
    //     case 14:
    //     case 15:
    // }
    // if (channel == 2) {

    // }

}

//
// midi delegates & handlers    //////////////////////////////////////////////////////////
//

void midiBegin() {
    #ifndef LOGGING
    MIDI.begin(MIDI_CHANNEL_OMNI);
    #endif

    logger("MIDI.begin()"); logger("\n");
}

void attachHandlers() {
    #ifndef LOGGING
    MIDI.setHandleNoteOn(handleNoteOn);
    #endif

    logger("attachHandlers()"); logger("\n");
}

void allOff() {
    #ifndef LOGGING
    if (!rSettings.shouldIPlay) {
        for (int c = 0; c < 16; c++) {
            MIDI.sendControlChange(123,0,c);
        }
        rSettings.shouldIPlay = true;
    }
    #endif

    logger("allOff()");
    logger("\n");
}

void noteOn(byte noteNumber, byte velocity, byte channel) {
    #ifndef LOGGING
    MIDI.sendNoteOn(noteNumber, velocity, channel);    // Send a Note 
    #endif

    logger("noteNumber ON:");
    logger(noteNumber);
    logger("\n");
}

void noteOff(byte noteNumber, byte velocity, byte channel) {
    #ifndef LOGGING
    MIDI.sendNoteOff(noteNumber, 0, channel);     // Stop the note
    #endif

    logger("noteNumber OFF:");
    logger(noteNumber);
    logger("\n");
}
