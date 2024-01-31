#pragma once

#define FASTLED_INTERNAL
#define FASTLED_USE_PROGMEM 1

#include <FastLED.h>

#include "constants.h"

class Led {
    int _width;
    int _height;
    int _count;

    CRGB *_data;

public:
    Led(int width, int height);
    ~Led();

    void set_pixel(int x, int y, const CRGB &color);
    void fill_column(int x, const CRGB &color);
    inline void fill_solid(const CRGB &color) { ::fill_solid(_data, _count, color); };

    [[nodiscard]] inline int width() const { return _width; }
    [[nodiscard]] inline int height() const { return _height; }
    [[nodiscard]] inline int count() const { return _count; }

    [[nodiscard]] inline int get_index(int x, int y) const {
        return (y & 1 ? _width - 1 - x : x) + y * _width;
    }

    inline CRGB &get_pixel(int x, int y) { return _data[get_index(x, y)]; }
    inline CRGB *data() { return _data; }

    inline void clear() { FastLED.clear(); }
    inline void show() { FastLED.show(); }
    inline void apply_gamma_correction(float value) { napplyGamma_video(_data, _count, value); }

    void fade_to_black(byte factor);
    inline void blur2d(uint8_t width, uint8_t height, uint8_t amount) { ::blur2d(_data, width, height, amount); }

    inline void set_correction(const CRGB &correction) { FastLED.setCorrection(correction); }
    inline void set_brightness(uint8_t scale) { FastLED.setBrightness(scale); }
    inline void set_power_limit(uint8_t voltage, uint32_t current) {
        FastLED.setMaxPowerInVoltsAndMilliamps(voltage, current);
    }
};