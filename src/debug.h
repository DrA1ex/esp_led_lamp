#pragma once

#ifdef DEBUG
#define D_PRINT(x) Serial.println(x)
#else
#define D_PRINT(x)
#endif