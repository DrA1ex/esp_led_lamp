#pragma once

#include "c_types.h"

enum class PacketType : uint8_t {
    SPEED = 0,
    SCALE = 1,
    LIGHT = 2,

    MAX_BRIGHTNESS = 20,

    PALETTE = 100,
    COLOR_EFFECT = 101,
    BRIGHTNESS_EFFECT = 102,

    CONFIGURE_PALETTE = 120,

    PALETTE_LIST = 140,
    COLOR_EFFECT_LIST = 141,
    BRIGHTNESS_EFFECT_LIST = 142,

    GET_CONFIG = 160,

    DISCOVERY = 200,

    POWER_OFF = 220,
    POWER_ON = 221,
    RESTART = 222,
};

struct __attribute__ ((packed))  PacketHeader {
    uint16_t signature;
    PacketType type;
    uint8_t size;
};