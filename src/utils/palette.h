#pragma once

#include "misc/led.h"

CRGB color_from_palette(const CRGBPalette16 &pal, uint16_t index, uint8_t scale = 255);

uint16_t inoise_hd(double x, double y, double z);
uint16_t inoise_hd(double x, double y);

float sin8f(float i);
float sin16f(float i);