#pragma once

#include <cstdint>
#include <cmath>

inline uint16_t map16(uint16_t value, uint16_t limit_src, uint16_t limit_dst) {
    value = std::max((uint16_t) 0, std::min(limit_src, value));
    return (int32_t) value * limit_dst / limit_src;
}