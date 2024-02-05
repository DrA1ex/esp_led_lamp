#pragma once

#include "misc/led.h"

CRGB color_from_palette(const CRGBPalette16 &pal, uint16_t index, uint8_t scale = 255);

uint16_t inoise_hd(double x, double y, double z);
uint16_t inoise_hd(double x, double y);

float sin8f(float i);
float sin16f(float i);

void set_palette(CRGBPalette16 &pal, const uint32_t *pgm_colors, size_t size);
uint32_t blend(uint32_t rgb1, uint32_t rgb2, uint8_t factor);
