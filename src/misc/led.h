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

    [[nodiscard]] inline int _get_index(int x, int y) const {
        return (y & 1 ? _width - 1 - x : x) + y * _width;
    }

public:
    Led(int width, int height);
    ~Led();

    void setPixel(int x, int y, const CRGB &color);
    void fillColumn(int x, const CRGB &color);
    inline void fillSolid(const CRGB &color) { fill_solid(_data, _count, color); };

    [[nodiscard]] inline int width() const { return _width; }
    [[nodiscard]] inline int height() const { return _height; }
    [[nodiscard]] inline int count() const { return _count; }

    inline CRGB &getPixel(int x, int y) { return _data[_get_index(x, y)]; }
    inline CRGB *data() { return _data; }

    inline void clear() { FastLED.clear(); }
    inline void show() { FastLED.show(); }
    inline void apply_gamma_correction(float value) { napplyGamma_video(_data, _count, value); }

    void fadeToBlack(byte factor);

    inline void setCorrection(const CRGB &correction) { FastLED.setCorrection(correction); }
    inline void setBrightness(uint8_t scale) { FastLED.setBrightness(scale); }
    inline void setPowerLimit(uint8_t voltage, uint32_t current) {
        FastLED.setMaxPowerInVoltsAndMilliamps(voltage, current);
    }
};