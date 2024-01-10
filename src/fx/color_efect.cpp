#include "color_effect.h"

void perlin(Led &led, const CRGBPalette16 &palette, byte scale, byte speed) {
    const auto height = led.height();
    const auto width = led.width();

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

void solid(Led &led, const CRGBPalette16 &, byte scale, byte speed) {
    auto color = CHSV(scale, speed, 255);

    fill_solid(led.data(), led.count(), color);
}

void changeColor(Led &led, const CRGBPalette16 &palette, byte, byte speed) {
    auto color = ColorFromPalette(palette, millis() / 20 * speed / 255, 255, LINEARBLEND);

    fill_solid(led.data(), led.count(), color);
}

void gradient(Led &led, const CRGBPalette16 &palette, byte scale, byte speed) {
    auto time_factor = (millis() >> 3) * (speed - 128) / 128;
    auto scale_factor = ((float) scale * 1.9f + 25) / (float) led.width();

    for (int i = 0; i < led.width(); i++) {
        auto index = (long) ((float) i * scale_factor) + time_factor;
        auto color = ColorFromPalette(palette, index, 255, LINEARBLEND);
        led.fillColumn(i, color);
    }
}

struct Particle {
    int x = 0;
    int y = 0;
    byte brightness = 0;
    CRGB color;
};

const int MAX_PARTICLES_COUNT = 50;
static Particle particles_store[MAX_PARTICLES_COUNT];

void particles(Led &led, const CRGBPalette16 &palette, byte scale, byte speed) {
    const auto height = led.height();
    const auto width = led.width();

    const int particle_count = 1 + scale / 255.0 * (MAX_PARTICLES_COUNT - 1);
    const int fade_speed = 1 + speed / 8;

    for (int k = 0; k < particle_count; ++k) {
        auto &particle = particles_store[k];

        if (particle.brightness == 0 && random8() < 32) {
            particle.x = random16() % width;
            particle.y = random16() % height;

            particle.color = ColorFromPalette(palette, random8(), 255, LINEARBLEND);
            particle.brightness = 255;
        }

        const auto color = particle.color.scale8(particle.brightness);
        auto &pixel = led.getPixel(particle.x, particle.y);
        nblend(pixel, color, 128);

        particle.brightness = ((uint16_t) particle.brightness * (255 - fade_speed)) >> 8;
    }
}

void _pacific_wave(Led &led, CRGB color, uint16_t phase, uint16_t width) {
    const auto from = max(0, phase - width / 2);
    const auto to = min(led.width(), phase + width / 2);

    for (int i = from; i < to; ++i) {
        const byte brightness = ease8InOutCubic(255 * abs(phase - i) / (width / 2));

        const auto &pixel = led.getPixel(i, 0);

        const auto faded_color = color.scale8(255 - brightness);
        const auto out_color = blend(pixel, faded_color, 32);

        led.setPixel(i, 0, out_color);
    }
}

void pacific(Led &led, const CRGBPalette16 &palette, byte scale, byte speed) {
    _pacific_wave(led,
                  ColorFromPalette(palette, beatsin8(speed / 16)),
                  beatsin16(speed / 8, 0, led.width(), 0),
                  scale / 8);

    _pacific_wave(led,
                  ColorFromPalette(palette, beatsin8(speed / 15)),
                  beatsin16(speed / 8 + 2, 0, led.width()),
                  scale / 8);

    _pacific_wave(led,
                  ColorFromPalette(palette, beatsin8(speed / 32)),
                  beatsin16(speed / 16, 0, led.width()),
                  scale / 4);

    _pacific_wave(led,
                  ColorFromPalette(palette, beatsin8(speed / 64)),
                  beatsin16(speed / 32, 0, led.width()),
                  scale / 2);

    for (int i = 0; i < led.width(); ++i) {
        led.fillColumn(i, led.getPixel(i, 0));
    }
}