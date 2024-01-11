#pragma once

#ifdef DEBUG
#define D_PRINT(x) Serial.println(x)
#define D_PRINTF(...) Serial.printf(__VA_ARGS__)
#define D_WRITE(x) Serial.print(x)
#else
#define D_PRINT(x)
#define D_PRINTF(...)
#define D_WRITE(x)
#endif