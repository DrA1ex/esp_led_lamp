#include "palette.h"

uint16_t inoise_hd(double x, double y) {
    return inoise16((uint32_t) (x * 256), (uint32_t) (y * 256)) / 16;
}

uint16_t inoise_hd(double x, double y, double z) {
    return inoise16((uint32_t) (x * 256), (uint32_t) (y * 256), (uint32_t) (z * 256)) / 16;
}

CRGB color_from_palette(const CRGBPalette16 &pal, uint16_t index, uint8_t scale) {
    constexpr uint8_t COUNT = 16;
    constexpr int32_t VALUES_PER_COLOR = 256;

    uint16_t color_index = index / VALUES_PER_COLOR;
    uint8_t fraction = (index % VALUES_PER_COLOR);

    const uint8_t current_index = color_index % COUNT;
    const uint8_t next_index = (current_index + 1) % COUNT;

    auto color = blend(pal[current_index], pal[next_index], fraction);;
    if (scale != 255) color.nscale8(scale);

    return color;
}
