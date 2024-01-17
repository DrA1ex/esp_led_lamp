#pragma once

#include "c_types.h"

enum class PacketType : uint8_t {
    SPEED = 0,
    SCALE = 1,
    LIGHT = 2,

    MAX_BRIGHTNESS = 20,
    ECO_LEVEL = 21,

    NIGHT_MODE_ENABLED = 40,
    NIGHT_MODE_BRIGHTNESS = 41,
    NIGHT_MODE_ECO = 42,
    NIGHT_MODE_START = 43,
    NIGHT_MODE_END = 44,
    NIGHT_MODE_INTERVAL = 45,

    PALETTE = 100,
    COLOR_EFFECT = 101,
    BRIGHTNESS_EFFECT = 102,

    CALIBRATION_R = 120,
    CALIBRATION_G = 121,
    CALIBRATION_B = 122,

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