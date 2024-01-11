#pragma once

#include "c_types.h"

enum PacketType : uint8_t {
    SPEED = 0,
    SCALE = 1,
    LIGHT = 2,

    MAX_BRIGHTNESS = 50,

    PALETTE = 100,
    COLOR_EFFECT = 101,
    BRIGHTNESS_EFFECT = 102,

    DISCOVERY = 250,
    POWER_OFF = 251,
    POWER_ON = 252,
    RESTART = 253,
};

struct __attribute__ ((packed))  PacketHeader {
    uint16_t signature;
    PacketType type;
    uint8_t size;
};