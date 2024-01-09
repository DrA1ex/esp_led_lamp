#pragma once

#include "config.h"
#include "led.h"

void perlin(Led &led, const CRGBPalette16 &palette, byte scale, byte speed);
void solid(Led &led, const CRGBPalette16 &_, byte scale, byte speed);
void changeColor(Led &led, const CRGBPalette16 &palette, byte scale, byte speed);
void gradient(Led &led, const CRGBPalette16 &palette, byte scale, byte speed);
