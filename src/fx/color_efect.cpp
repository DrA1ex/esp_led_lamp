#include "color_effect.h"

#include "misc/led.h"

ColorEffectManager::ColorEffectManager() {
    _config.entries = {
            {ColorEffectEnum::PERLIN,       "Perlin Noise", perlin},
            {ColorEffectEnum::GRADIENT,     "Gradient",     gradient},
            {ColorEffectEnum::PACIFIC,      "Pacific",      pacific},
            {ColorEffectEnum::PARTICLES,    "Particles",    particles},
            {ColorEffectEnum::CHANGE_COLOR, "Color Change", changeColor},
            {ColorEffectEnum::SOLID,        "Solid Color",  solid},
    };

    _config.count = _config.entries.size();
}

void ColorEffectManager::call(Led &led, const CRGBPalette16 *palette, const Config &config) {
    _before_call();

    _state.params.speed = config.speed;
    _state.params.scale = config.scale;
    _state.params.palette = palette;

    _config.entries[(int) _fx].value(led, _state);

    _save_next_value(_state.prev_scale_value, _state.current_scale_value);
    _save_next_value(_state.prev_speed_value, _state.current_speed_value);
    _after_call();
}

void ColorEffectManager::_reset_state() {
    _state.current_speed_value = 0;
    _state.current_scale_value = 0;
    _state.prev_speed_value = 0;
    _state.prev_scale_value = 0;
}

void ColorEffectManager::perlin(Led &led, ColorEffectState &state) {
    const auto &[
            palette,
            scale,
            speed
    ] = state.params;

    const auto height = led.height();
    const auto width = led.width();

    state.current_speed_value = state.prev_speed_value + (float) state.delta() * speed / 4 / 255;
    const auto time_factor = _apply_period(state.current_speed_value, (1 << 16) - 1);

    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            led.setPixel(i, j, ColorFromPalette(
                    *palette,
                    inoise8(
                            i * (scale / 5) - width * (scale / 5) / 2,
                            j * (scale / 5) - height * (scale / 5) / 2,
                            time_factor),
                    255, LINEARBLEND));
        }
    }
}

void ColorEffectManager::solid(Led &led, ColorEffectState &state) {
    const auto &[
            palette,
            scale,
            speed
    ] = state.params;

    auto color = CHSV(speed, scale, 255);

    led.fillSolid(color);
}

void ColorEffectManager::changeColor(Led &led, ColorEffectState &state) {
    const auto &[
            palette,
            scale,
            speed
    ] = state.params;

    state.current_speed_value = state.prev_speed_value + (float) state.delta() * speed / 10.f / 255.f;
    const byte value = _apply_period(state.current_speed_value, 256);

    auto color = ColorFromPalette(*palette, value, 255, LINEARBLEND);
    led.fillSolid(color);
}

void ColorEffectManager::gradient(Led &led, ColorEffectState &state) {
    const auto &[
            palette,
            scale,
            speed
    ] = state.params;

    state.current_speed_value = state.prev_speed_value + (float) state.delta() * (speed - 128) / 8.f / 128.f;
    const auto time_factor = _apply_period(state.current_speed_value, 256);

    auto scale_factor = ((float) scale * 1.9f + 25) / (float) led.width();

    for (int i = 0; i < led.width(); i++) {
        auto index = (long) ((float) i * scale_factor) + time_factor;
        auto color = ColorFromPalette(*palette, index % 256, 255, LINEARBLEND);
        led.fillColumn(i, color);
    }
}

struct Particle {
    int x = 0;
    int y = 0;
    byte brightness = 0;
    CRGB color = CRGB::Black;
};

static Particle particles_store[MAX_PARTICLES_COUNT];

void ColorEffectManager::particles(Led &led, ColorEffectState &state) {
    const auto &[
            palette,
            scale,
            speed
    ] = state.params;

    const auto height = led.height();
    const auto width = led.width();

    const uint16_t particle_count = 1 + scale * (MAX_PARTICLES_COUNT - 1) / 255;
    const int fade_speed = 1 + speed / 8;

    for (uint16_t k = 0; k < particle_count; ++k) {
        auto &particle = particles_store[k];

        if (particle.brightness == 0 && random8() < 32) {
            particle.x = random16() % width;
            particle.y = random16() % height;

            particle.color = ColorFromPalette(*palette, random8(), 255, LINEARBLEND);
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
        const auto out_color = blend(pixel, faded_color, 64);

        led.setPixel(i, 0, out_color);
    }
}

void ColorEffectManager::pacific(Led &led, ColorEffectState &state) {
    const auto &[
            palette,
            scale,
            speed
    ] = state.params;

    _pacific_wave(led,
                  ColorFromPalette(*palette, beatsin8(speed / 16)),
                  beatsin16(speed / 8, 0, led.width(), 0),
                  scale / 8);

    _pacific_wave(led,
                  ColorFromPalette(*palette, beatsin8(speed / 15)),
                  beatsin16(speed / 8 + 2, 0, led.width()),
                  scale / 8);

    _pacific_wave(led,
                  ColorFromPalette(*palette, beatsin8(speed / 32)),
                  beatsin16(speed / 16, 0, led.width()),
                  scale / 4);

    _pacific_wave(led,
                  ColorFromPalette(*palette, beatsin8(speed / 64)),
                  beatsin16(speed / 32, 0, led.width()),
                  scale / 2);

    for (int i = 0; i < led.width(); ++i) {
        led.fillColumn(i, led.getPixel(i, 0));
    }
}
