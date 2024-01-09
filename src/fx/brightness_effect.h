#pragma once

#include "debug.h"
#include "led.h"

void fixed(Led &led, byte level);
void pulse(Led &led, byte level);
void wave(Led &led, byte level);