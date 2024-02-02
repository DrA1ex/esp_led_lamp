#pragma once

#include "constants.h"

#if BUTTON == ENABLED
#define BUTTON_FN(x) (x)
#else
#define BUTTON_FN(x)
#endif

constexpr float gamma_value(uint8_t x) { return (2.2f + (float) (x - 128) / 128.f); }