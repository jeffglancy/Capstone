// Modified from Adafruit Arduino library
// Jeff Glancy
// 9/4/14

#include <bcm2835.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#ifndef LPD8806_H
#define LPD8806_H

extern void LPD8806init(uint16_t n); // Use SPI hardware; specific pins only

extern void LPD8806quit();

extern void LPD8806setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b);

extern void LPD8806setPixelColor2(uint16_t n, uint32_t c);

extern void LPD8806show(void);

extern void LPD8806updateLength(uint16_t n);               // Change strip length

extern uint16_t LPD8806numPixels(void);

extern uint32_t LPD8806Color(uint8_t, uint8_t, uint8_t);

extern uint32_t LPD8806getPixelColor(uint16_t n);

#endif