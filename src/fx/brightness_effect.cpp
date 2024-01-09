#include "brightness_effect.h"

void fixed(Led &led, byte level) {
    led.fadeToBlack(255 - level);
}

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
    D_PRINT(String("Pulse phase: ") + value + " (brightness: " + String(brightness / 2.55f, 1) + "%)");

    led.fadeToBlack(255 - brightness);
}

void wave(Led &led, byte level) {
    const auto offset = millis() * 8 / (1 + 255 - level);

    for (int i = 0; i < led.width(); ++i) {
        const auto brightness = cubicwave8((offset + i * 6) % 256);

        for (int j = 0; j < led.height(); ++j) {
            auto &color = led.getPixel(i, j);
            color.fadeToBlackBy(255 - brightness);
        }
    }
}
