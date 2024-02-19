#include "led.h"

Led::Led(int width, int height) : _width(width), _height(height) {
    _count = width * height;
    _data = new CRGB[_count];

    FastLED.addLeds<LED_TYPE, LED_PIN, LED_COLOR_ORDER>(_data, _count);
}

Led::~Led() {
    delete[] _data;
}

void Led::set_pixel(int x, int y, const CRGB &color) {
    if (x < 0 || x >= _width || y < 0 || y >= _height) return;

    _data[get_index(x, y)] = color;
}

void Led::fill_column(int x, const CRGB &color) {
    if (x < 0 || x >= _width) return;

    for (int j = 0; j < _height; ++j) {
        _data[get_index(x, j)] = color;
    }
}

void Led::fade_to_black(byte factor) {
    for (int i = 0; i < _count; ++i) {
        _data[i].fadeToBlackBy(factor);
    }
}
