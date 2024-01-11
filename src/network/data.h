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

    CONFIGURE_PALETTE = 150,

    DISCOVERY = 200,

    PALETTE_LIST = 210,
    COLOR_EFFECT_LIST = 211,
    BRIGHTNESS_EFFECT_LIST = 212,

    POWER_OFF = 220,
    POWER_ON = 221,
    RESTART = 222,
};

struct __attribute__ ((packed))  PacketHeader {
    uint16_t signature;
    PacketType type;
    uint8_t size;
};