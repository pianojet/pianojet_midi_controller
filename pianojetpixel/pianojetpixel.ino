// https://www.arduino.cc/en/Tutorial/MasterReader
// https://www.arduino.cc/en/Reference/Wire

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <algorithm>

#include "../colors.h"

#define RIBBON_LED_PIN 2
#define RIBBON_LED_LEN 3

#define KEY_LED_PIN 23
#define KEY_LED_LEN 49

#define NUM_PIXEL_STRIPS 2

#define BUS_ADDRESS 8
#define NUM_CHANNELS 16


uint32_t colorChannelMap[NUM_CHANNELS];

struct StripNode {
    StripNode * next;
    uint8_t channelNum;
    uint8_t noteNum;
    uint32_t color;
    StripNode() {
        this->channelNum = 255;
        this->noteNum = 0;
        this->next = NULL;
    }
};

struct TxColorWord {
    byte header;
    byte stripNum;
    byte channelNum;
    byte noteNum;
    char action; // 'n' = on, 'f' = off, 'a', 'b', 'c', 'd', 'e' ... = mode
};


// SORT these later for better performance
StripNode * ribbonLEDNodes[RIBBON_LED_LEN];
StripNode * keyLEDNodes[KEY_LED_LEN];

// containers for pixel strip lists
Adafruit_NeoPixel * strips[NUM_PIXEL_STRIPS];
StripNode * stripNodes[NUM_PIXEL_STRIPS];

bool dumbToggle = false;


const byte txWordHeader = 0xFF;
const TxColorWord blankWord = {0xFF, 0xFF, 0xFF, 0xFF};
TxColorWord colorWord;

const uint16_t MAX_LED_LEN = getMaxLEDs();

void getMaxLEDs() {
    uint16_t max = 0;
    for(byte i = 0; i < NUM_PIXEL_STRIPS; i++) {
        if (strips[i]->numPixels() > max) max = strips[i]->numPixels();
    }
}

colorChannelMap[0] = WHITE;
colorChannelMap[1] = RED;
colorChannelMap[2] = GREEN;
colorChannelMap[3] = BLUE;
colorChannelMap[4] = RED;
colorChannelMap[5] = GREEN;
colorChannelMap[6] = BLUE;
colorChannelMap[7] = RED;
colorChannelMap[8] = GREEN;
colorChannelMap[9] = BLUE;
colorChannelMap[10] = RED;
colorChannelMap[11] = GREEN;
colorChannelMap[12] = BLUE;
colorChannelMap[13] = RED;
colorChannelMap[14] = GREEN;
colorChannelMap[15] = BLUE;


void initialize() {
    // SORT these for better performance
    // sorting brute force for now, longer strips first
    strips[0] = new Adafruit_NeoPixel(KEY_STRIP_LEN, KEY_STRIP_PIN, NEO_GRB + NEO_KHZ800);
    strips[1] = new Adafruit_NeoPixel(RIBBON_STRIP_LEN, RIBBON_LED_PIN, NEO_GRB + NEO_KHZ800);

    stripNodes[0] = keyLEDNodes;
    stripNodes[1] = ribbonLEDNodes;

    for(byte i = 0; i < KEY_LED_LEN; i++) {
        StripNode * node = new StripNode();
        node->noteNum = i;
        node->channelNum = 255;
        node->color = BLACK;
        keyLEDNodes[i] = node;
    }

    for(byte i = 0; i < RIBBON_LED_LEN; i++) {
        StripNode * node = new StripNode();
        node->noteNum = i;
        node->channelNum = 255;
        node->color = BLACK;
        ribbonLEDNodes[i] = node;
    }

}

void insertColor(uint32_t color, ) {}


void paintStrips() {
    // Adafruit_NeoPixel * stripsRemaining[NUM_PIXEL_STRIPS] = strips
    uint8_t numStrips = NUM_PIXEL_STRIPS;
    // Adafruit_NeoPixel * paintingStrip

    // while()


    StripNode * thisNode = NULL;
    for (byte i = 0; i < MAX_LED_LEN; i++)
    {
        for (byte j = 0; j < numStrips; j++) {

            thisNode = stripNodes[j][i];

            // account for a key to be claimed by multiple channels (colors)
            while (thisNode = thisNode.next) {}

            strips[j]->setPixelColor(i, thisNode->color);

            if (strips[j]->numPixels() == (i+1)) numStrips--;
        }

    }

    for (byte k = 0; k < NUM_PIXEL_STRIPS; k++) {
        strips[k]->show();
    }

}


void receiveEvent() {
    StripNode * newNode, thisNode;

    while (Wire.available()) {
        colorWord.header = Wire.read();

        if (colorWord.header == txWordHeader && Wire.available() >= 3) {
            colorWord.stripNum = Wire.read();
            colorWord.channelNum = Wire.read();
            colorWord.noteNum = Wire.read();
            colorWord.action = Wire.read();

            StripNode * newNode = new StripNode();
            newNode->channelNum = colorWord.channelNum;
            newNode->noteNum = colorWord.noteNum;


            switch (colorWord.action) {
                case 'f':
                    newNode->color = BLACK;
                break;

                case 'n':
                    newNode->color = colorChannelMap[newNode->channelNum];
                break;

                case 'a':
                break;

                case 'b':
                break;

                case 'c':
                break;
            }

            // if (colorWord.action == 'n')
            //     newNode->color = colorChannelMap[newNode->channelNum];
            // } else {
            //     newNode->color = BLACK;
            // }


            // switch (colorWord.stripNum) {
            //     // keyboard pixel strip
            //     case 0:
            //         if (colorWord.action == 'n')
            //             newNode->color = colorChannelMap[newNode->channelNum];
            //         } else {
            //             newNode->color = BLACK;
            //         }

            //     break;

            //     // ribbon pixel strip
            //     case 1:
            //     break;
            // }

            
        }
    }
}

uint8_t loopCount = 0;
uint8_t _draw_h = 0;
uint8_t _draw_i = 0;
uint8_t _draw_j = 0;
uint8_t _draw_k = 0;


void theaterChase() {
    if (_draw_h < 10) {
        if (_draw_i < 3) {

        }
    }
}


setup() {
    Serial.begin(9600);
    Serial.println("STARTING!\n");

    initialize();

    Wire.begin(BUS_ADDRESS);
    Wire.onReceive(receiveEvent);

    strip.begin();
    strip.setBrightness(50);
    strip.show();
}


loop() {
    if (dumbToggle) {

    }
    // theaterChase(WHITE, 50);

    loopCount++;
    if (loopCount > 99) loopCount = 0;
}


// //Theatre-style crawling lights.
// void theaterChase(uint32_t c, uint8_t wait) {




//   for (int j=0; j<10; j++) {  //do 10 cycles of chasing
//     for (int q=0; q < 3; q++) {
//       for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
//         strip.setPixelColor(i+q, c);    //turn every third pixel on
//       }
//       strip.show();

//       delay(wait);

//       for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
//         strip.setPixelColor(i+q, 0);        //turn every third pixel off
//       }
//     }
//   }
// }

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