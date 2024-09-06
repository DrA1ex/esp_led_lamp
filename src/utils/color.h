#pragma once

#include <algorithm>
#include <cstdint>
#include <cmath>

struct HSL {
    uint8_t h;
    uint8_t s;
    uint8_t l;
};

inline HSL RGBToHSL(uint32_t rgb) {
    // Extract the red, green, and blue components
    uint8_t r = (rgb >> 16) & 0xFF;
    uint8_t g = (rgb >> 8) & 0xFF;
    uint8_t b = rgb & 0xFF;

    // Normalize the values to [0, 1]
    float rN = r / 255.0f;
    float gN = g / 255.0f;
    float bN = b / 255.0f;

    float maxC = std::max({ rN, gN, bN });
    float minC = std::min({ rN, gN, bN });
    float delta = maxC - minC;

    float h = 0, s = 0, l = (maxC + minC) / 2;

    if (delta > 0) {
        if (maxC == rN) {
            h = fmod(((gN - bN) / delta), 6.0);
        } else if (maxC == gN) {
            h = ((bN - rN) / delta) + 2.0;
        } else {
            h = ((rN - gN) / delta) + 4.0;
        }
        h *= 60.0;
        if (h < 0) h += 360;

        s = (maxC == 0) ? 0 : (delta / (1 - std::abs(2 * l - 1)));
    }

    return HSL{
            static_cast<uint8_t>(h / 360.0 * 255), // Normalize h to [0, 255]
            static_cast<uint8_t>(s * 255),         // Normalize s to [0, 255]
            static_cast<uint8_t>(l * 255)           // Normalize l to [0, 255]
    };
}

inline uint32_t HSLToRGB(const HSL& hsl) {
    float h = hsl.h / 255.0f * 360; // Normalize back to [0, 360]
    float s = hsl.s / 255.0f;        // Normalize to [0, 1]
    float l = hsl.l / 255.0f;        // Normalize to [0, 1]

    float c = (1 -std::abs(2 * l - 1)) * s; // Chroma
    float x = c * (1 - std::abs(fmod(h / 60.0, 2) - 1));
    float m = l - (c / 2);

    float rN, gN, bN;
    if (h < 60) {
        rN = c; gN = x; bN = 0;
    } else if (h < 120) {
        rN = x; gN = c; bN = 0;
    } else if (h < 180) {
        rN = 0; gN = c; bN = x;
    } else if (h < 240) {
        rN = 0; gN = x; bN = c;
    } else if (h < 300) {
        rN = x; gN = 0; bN = c;
    } else {
        rN = c; gN = 0; bN = x;
    }

    uint8_t r = static_cast<uint8_t>((rN + m) * 255);
    uint8_t g = static_cast<uint8_t>((gN + m) * 255);
    uint8_t b = static_cast<uint8_t>((bN + m) * 255);

    return (r << 16) | (g << 8) | b; // Combine r, g, b into a uint32_t
}