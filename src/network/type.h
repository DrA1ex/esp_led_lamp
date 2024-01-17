#pragma once

#include "c_types.h"

#include "enum.h"

struct __attribute__ ((packed))  PacketHeader {
    uint16_t signature;
    PacketType type;
    uint8_t size;
};