#include "brightness_effect.h"

void fixed(Led &led, byte level) {}

void pulse(Led &led, byte level) {
    auto value = (millis() * 4 / (1 + 255 - level)) % 768;

    byte phase;
    if (value < 256) { // Fade in
        phase = value;
    } else if (value < 512) { // Active
        phase = 255;
    } else { // Fade out
        phase = 768 - value - 1;
    }

    auto brightness = ease8InOutQuad(phase);
    led.fadeToBlack(255 - brightness);
}

void wave(Led &led, byte level) {
    const auto offset = millis() * 8 / (1 + 255 - level);

    for (int i = 0; i < led.width(); ++i) {
        const auto brightness = cubicwave8((offset + i * 6) % 256);

        for (int j = 0; j < led.height(); ++j) {
            auto &color = led.getPixel(i, j);
            color.fadeToBlackBy(brightness);
        }
    }
}

void double_wave(Led &led, byte level) {
    const auto &width = led.width();

    const auto offset = millis() * 8 / (1 + 255 - level);
    const int half_width = ceil(width / 2.0f);

    for (int i = 0; i < half_width; ++i) {
        const auto phase = (offset + i * 32) % 512;
        const byte brightness = 255 - (phase < 256 ? cubicwave8(phase) : 0);

        for (int j = 0; j < led.height(); ++j) {
            auto &color = led.getPixel(i, j);
            color.fadeToBlackBy(brightness);

            const auto i2 = width - i - 1;
            if (i2 >= half_width) {
                auto &color2 = led.getPixel(i2, j);
                color2.fadeToBlackBy(brightness);
            }
        }
    }
}
