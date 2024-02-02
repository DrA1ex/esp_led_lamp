#pragma once

#include "constants.h"

#if BUTTON == ENABLED
#define BUTTON_FN(x) (x)
#else
#define BUTTON_FN(x)
#endif

#define GAMMA_V(x) (2.2f + (float) (x - 128) / 128.f)