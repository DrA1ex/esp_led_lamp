#pragma once

#include "constants.h"

#if BUTTON == ENABLED
#define BUTTON_FN(x) (x)
#else
#define BUTTON_FN(x)
#endif