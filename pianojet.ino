#include <Arduino.h>
#include <Key.h>
#include <Keypad.h>

#include <ClickEncoder.h>
#include <MIDI.h>

#define POTPIN 14  //wiper

#define LOGGING true

//
// definitions & types //////////////////////////////////////////////////////////
//

//
// Ribbon
//
const byte DEFAULT_RIBBON_VELOCITY = 100;
struct RibbonControllerSettings {
    uint16_t 
        lastWiperState,
        analogOffset;

    uint8_t
        lastWiperNote,
        windowRadius,
        analogCutoff,
        analogScaleFactor,
        
        velocity01,
        velocity02,

        channel01,
        channel02;

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
void voidFn
public:
    int16_t defaultValue;

    void processButton(ClickEncoder::Button button);
    void processValue(int16_t value);
    void setButtonFn(void (*buttonFn)(ClickEncoder::Button));
    void setValueFn(void (*valueFn)(int16_t));

    RotaryControllerAdapter();
    ~RotaryControllerAdapter();
private:
    void (*_processButton)(RotaryControllerAdapter, ClickEncoder::Button);
    void (*_processValue)(RotaryControllerAdapter, int16_t);
}
void RotaryControllerAdapter::RotaryControllerAdapter(void (*buttonFn)(ClickEncoder::Button), void (*valueFn)(int16_t), int16_t defaultValue) {
    this.defaultValue = defaultValue;
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

// RotaryController
class RotaryController {
public:
    uint8_t
        currentValue,
        lastValue;
    RotaryControllerAdapter adapter;
    ClickEncoder *encoder;
    void tick();
    RotaryController();
    ~RotaryController();
}
void RotaryController::RotaryController(uint8_t rotaryPin01, uint8_t rotaryPin02, uint8_t buttonPin, RotaryControllerAdapter adapter) {
    this->encoder = new ClickEncoder(rotaryPin01, rotaryPin02, buttonPin);
    this.adapter = adapter;
}
void RotaryController::tick() {
    this.adapter.processValue(this->encoder->getValue());
    this.adapter.processButton(this->encoder->getButton());
}

//
// declarations & initializations //////////////////////////////////////////////////////////
//

RibbonControllerSettings rSettings;

KeypadControllerSettings kSettings;
Keypad * keypad = NULL;

void ribbonVelocityButton(RotaryControllerAdapter self, ClickEncoder::Button button) {
    logger("Rotary button:")
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

            rSettings.velocity01 = self.defaultValue;
            rSettings.velocity02 = self.defaultValue;

            logger("Velocity 01 new value: "); logger(rSettings.velocity01); logger("\n");
            logger("Velocity 02 new value: "); logger(rSettings.velocity02); logger("\n");
        break;

        case ClickEncoder::DoubleClicked:
            logger("  DoubleClicked\n");
        break;
    }
}
void ribbonVelocityValue(RotaryControllerAdapter self, int16_t value) {
    logger(sprintf("Rotary value: %d\n", value));

    int16_t newValue = self.defaultValue + value;
    if (newValue < 0) { newValue = 0;}
    if (newValue > 127) { newValue = 127;}

    rSettings.velocity01 = newValue;
    rSettings.velocity02 = newValue;

    logger("Velocity 01 new value: "); logger(rSettings.velocity01); logger("\n");
    logger("Velocity 02 new value: "); logger(rSettings.velocity02); logger("\n");

}
RotaryControllerAdapter ribbonAdapter(ribbonVelocityButton, ribbonVelocityValue, 100);
RotaryController *rotary[ROTARY_COUNT] = {NULL, NULL, NULL, NULL};

#ifndef LOGGING
MIDI_CREATE_DEFAULT_INSTANCE();
#endif

void pianojetInit() {
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
    for (int r = 0; r < ROTARY_COUNT; c++) {
        if (rotary[r]) {
            delete rotary[r];
        }
    }
    rotary[0] = new RotaryController(A1, A0, A2, ribbonAdapter);
}

//
// arduino setup & loop  //////////////////////////////////////////////////////////
//

void setup()
{
    Serial.begin(9600);
    Serial.println("STARTING!\n");
 
    pianojetInit();
 
    midiBegin();

    allOff();

    logger(rSettings);

}

void loop()
{
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

    for (int r = 0; r < ROTARY_COUNT; c++) {
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
    Serial.print(s);
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

void logger(const long n) {
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

    rSettings.velocity01 = DEFAULT_RIBBON_VELOCITY;
    rSettings.velocity02 = DEFAULT_RIBBON_VELOCITY;
    rSettings.channel01 = 1;
    rSettings.channel02 = 2;

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

    rSettings.velocity01 = DEFAULT_RIBBON_VELOCITY;
    rSettings.velocity02 = DEFAULT_RIBBON_VELOCITY;
    rSettings.channel01 = 1;
    rSettings.channel02 = 2;

    rSettings.pitchBendMode = true;
}

int readWiper() {
    return analogRead(POTPIN);
}

int scaleReadingNote(unsigned int reading) {
    return (int)(reading * 0.125); // 128 / 1016
}

int scaleReadingPitchBend(unsigned int reading) {
    return map(reading, 0, 1015, 0, 127);
}

void playWiper(unsigned int reading) {

    // pitchbend mode
    if (rSettings.pitchBendMode) {

        int localNote = scaleReadingPitchBend(reading);

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

        int localNote = scaleReadingNote(reading);

        if (reading > rSettings.analogCutoff) {

            if (localNote != rSettings.lastWiperNote) {
                noteOff(rSettings.lastWiperNote);
                noteOn(localNote);
                rSettings.shouldIPlay = false;

                rSettings.lastWiperNote = localNote;
            } 

            rSettings.lastWiperState = reading;

        } else {

            if (rSettings.shouldIPlay == false) {
                noteOff(rSettings.lastWiperNote);
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

uint16_t addZeroDigit(uint16_t number) {
    long added;
    char str1[16] = "";
    char str2[16] = "";

    // could do a recursion trick here

    // sprintf(str, "%d", (int)number);
    sprintf(str1, "%d0", (int)number);
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

    

    return (uint16_t)strtol(str1, NULL, 10);
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
// midi delegates & handlers    //////////////////////////////////////////////////////////
//

void midiBegin() {
    #ifndef LOGGING
    MIDI.begin();
    #endif

    logger("MIDI.begin()");
    logger("\n");
}

void allOff() {
    #ifndef LOGGING
    if (!shouldIPlay) {
        for (int c = 0; c < 16; c++) {
            MIDI.sendControlChange(123,0,c);
        }
        rSettings.shouldIPlay = true;
    }
    #endif

    logger("allOff()");
    logger("\n");
}

void noteOn(unsigned int noteNumber) {
    #ifndef LOGGING
    MIDI.sendNoteOn(noteNumber, 127, 1);    // Send a Note 
    #endif

    logger("noteNumber ON:");
    logger(noteNumber);
    logger("\n");
}

void noteOff(unsigned int noteNumber) {
    #ifndef LOGGING
    MIDI.sendNoteOff(noteNumber, 0, 1);     // Stop the note
    #endif

    logger("noteNumber OFF:");
    logger(noteNumber);
    logger("\n");
}
