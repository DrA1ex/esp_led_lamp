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

float sin8f(float i) {
    return sin16f(i * 256) / 65536 * 255;
}

float sin16f(float i) {
    const int32_t k = i / 65536;
    if (k != 0) {
        i -= k * 65536;
    }

    return sin16((uint16_t) round(i)) + 32767;
}

void set_palette(CRGBPalette16 &pal, const uint32_t *pgm_colors, size_t size) {
    const auto out_size = sizeof(pal.entries) / sizeof(pal.entries[0]);
    const auto step = (float) (size - 1) / (out_size - 1);

    for (size_t i = 0; i < out_size - 1; i++) {
        const auto index = (float) i * step;

        const size_t from = floor(index);
        const size_t to = from + 1;

        const float factor = (index - from) / (to - from);

        const auto from_color = pgm_read_dword(pgm_colors + from);
        const auto to_color = pgm_read_dword(pgm_colors + to);

        pal.entries[i] = blend(from_color, to_color, (uint8_t) (factor * 255));
    }

    pal.entries[out_size - 1] = pgm_read_dword(pgm_colors + (size - 1));
}

inline uint8_t extract_color(uint32_t color, uint8_t shift) {
    return (color >> shift) & 0xff;
}

inline void set_color(uint32_t &color, uint8_t value, uint8_t shift) {
    color |= ((uint32_t) value << shift);
}

uint8_t  _blend(uint8_t r1, uint8_t r2, uint8_t  factor) {
    return r1 - ((uint16_t)r1 - r2) * factor / 255;
}

uint32_t blend(uint32_t rgb1, uint32_t rgb2, uint8_t factor) {
    const uint8_t r1 = extract_color(rgb1, 16);
    const uint8_t g1 = extract_color(rgb1, 8);
    const uint8_t b1 = extract_color(rgb1, 0);

    const uint8_t r2 = extract_color(rgb2, 16);
    const uint8_t g2 = extract_color(rgb2, 8);
    const uint8_t b2 = extract_color(rgb2, 0);

    uint32_t result = 0;
    set_color(result, _blend(r1, r2, factor), 16);
    set_color(result, _blend(g1, g2, factor), 8);
    set_color(result, _blend(b1, b2, factor), 0);

    return result;
}
