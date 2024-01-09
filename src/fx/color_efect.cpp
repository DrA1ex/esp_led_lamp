#include "color_effect.h"

void perlin(Led &led, const CRGBPalette16 &palette, byte scale, byte speed) {
    const auto height = led.height();
    const auto width = led.width();

    led.clear();
    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            led.setPixel(i, j, ColorFromPalette(
                    palette,
                    inoise8(
                            i * (scale / 5) - width * (scale / 5) / 2,
                            j * (scale / 5) - height * (scale / 5) / 2,
                            (millis() / 2) * speed / 255),
                    255, LINEARBLEND));
        }
    }
}

void solid(Led &led, const CRGBPalette16 &_, byte scale, byte speed) {
    auto color = CHSV(scale, speed, 255);

    led.clear();
    fill_solid(led.data(), led.count(), color);
}

void changeColor(Led &led, const CRGBPalette16 &palette, byte scale, byte speed) {
    auto color = ColorFromPalette(palette, millis() / 20 * speed / 255, 255, LINEARBLEND);

    led.clear();
    fill_solid(led.data(), led.count(), color);
}

void gradient(Led &led, const CRGBPalette16 &palette, byte scale, byte speed) {
    auto time_factor = (millis() >> 3) * (speed - 128) / 128;
    auto scale_factor = ((float) scale * 1.9f + 25) / (float) led.width();

    led.clear();
    for (int i = 0; i < led.width(); i++) {
        auto index = (long) ((float) i * scale_factor) + time_factor;
        auto color = ColorFromPalette(palette, index, 255, LINEARBLEND);
        led.fillColumn(i, color);
    }
}
