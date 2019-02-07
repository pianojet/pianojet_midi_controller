#ifndef __FIREPIT_H__
#define __FIREPIT_H__

#include <Arduino.h>

const uint32_t BLACK = 0x000000;
const uint32_t RED = 0xFF0000;
const uint32_t GREEN = 0x00FF00;
const uint32_t LIGHTGREEN = 0xC8FFC8;
const uint32_t DARKGREEN = 0x005500;
const uint32_t BLUE = 0x0000FF;
const uint32_t LIGHTBLUE = 0x000055;
const uint32_t YELLOW = 0xFFFF00;
const uint32_t TEAL = 0x00FFFF;
const uint32_t PURPLE = 0xFF00FF;
const uint32_t ORANGE = 0xFFA500;
const uint32_t WHITE = 0xFFFFFF;



// Input a value 0 to 255 to get a color value.
// The colors are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
    if(WheelPos < 85) {
        return seesaw_NeoPixel::Color(WheelPos * 3, 255 - WheelPos * 3, 0);
    } else if(WheelPos < 170) {
    WheelPos -= 85;
        return seesaw_NeoPixel::Color(255 - WheelPos * 3, 0, WheelPos * 3);
    } else {
        WheelPos -= 170;
        return seesaw_NeoPixel::Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
    return 0;
}

#endif
