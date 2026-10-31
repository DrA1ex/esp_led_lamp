#pragma once
// Geometry-aware remote visualizations, AudioEffectEnum::VU_METER..CHROMATIC_FLOW
// (lab audio modes 13..31). The modes live in viz_engine.h, shared with the
// lab preview; this host feeds it the remote stream and draws with the
// current palette (or masks the colour effect with viz_color).
#include <FastLED.h>

#include "constants.h"
#include "misc/led.h"
#include "utils/palette.h"
#include "audio/transfer.h"
#include "reactive_math.h"
#include "controller.h"
#include "viz_engine.h"

namespace RemoteViz {
constexpr uint8_t FIRST = uint8_t(AudioEffectEnum::VU_METER);

inline bool handles(AudioEffectEnum mode) {
    return uint8_t(mode) >= FIRST && Viz::available(uint8_t(mode) - FIRST);
}

class Host {
    Led &_led;
    const RemoteAudioController &_controller;
    const PaletteT &_palette;
    const RemoteReactive::OriginMap _origin;
    const AudioWindow &_window;
    const uint8_t _out_min, _out_max;
    const bool _mask;
    const int _row; // radial centre row, Q8

public:
    Host(Led &led, const RemoteAudioController &controller, const PaletteT &palette, uint8_t v_min, uint8_t v_max) :
            _led(led), _controller(controller), _palette(palette),
            _origin(led.width(), led.height(), controller.overrides().tuning.origin),
            _window(controller.overrides().tuning.window), _out_min(v_min), _out_max(v_max),
            _mask(controller.overrides().tuning.viz_color != 0),
            _row(int32_t(led.height() - 1) * 256 * min<uint16_t>(controller.overrides().tuning.origin.center_y, 1000) / 1000) {}

    int width() const { return _led.width(); }
    int height() const { return _led.height(); }
    int originX(int x) const { return _origin.x(x); }
    bool radial() const { return _origin.radial(); }
    int radialRow() const { return _row; }
    // Reaction range, signal window and curve, like the lab's scaled_audio().
    // The engine works on Q12 levels, the same 0..4096 scale as the stream.
    int transfer(int v) const {
        return audioTransfer(v, RemoteProtocol::MAX_LEVEL, RemoteProtocol::MAX_LEVEL, _out_min, _out_max, _window);
    }
    int control() const { return _controller.controlValue(); }
    // The lab blends in its fast line envelope; the stream has no such field.
    int lineSource() const { return control(); }
    int level(int g) const {
        const auto &f = _controller.timeline().output();
        return g == 0 ? f.bass : g == 1 ? f.mid : f.high;
    }
    int pulse(int g) const { return _controller.pulses()[g]; }
    // The Beat control source in integers: a pulse fading 0.23 s after each
    // predicted beat (80 phase steps without a tempo), times the confidence.
    int beat() const {
        const auto &f = _controller.timeline().output();
        int32_t pulse = 4096 - int32_t(f.beat_phase) * 4096 / 80;
        if (f.bpm_x10) {
            const int32_t since_ms = int32_t(f.beat_phase) * 600000 / (256 * int32_t(f.bpm_x10));
            pulse = since_ms >= 230 ? 0 : 4096 - since_ms * 4096 / 230;
        }
        return pulse <= 0 ? 0 : pulse * f.beat_confidence / 255;
    }
    // The frame's beat phase (Q16 turn), carried on at the tempo for the frame's
    // age, so it moves between frames instead of in 50 Hz steps.
    int beatPhase() const {
        const auto &f = _controller.timeline().output();
        uint32_t phase = uint32_t(f.beat_phase) << 8;
        if (f.bpm_x10 && f.beat_confidence) {
            const uint32_t age = min<uint32_t>(_controller.timeline().age(), 300);
            phase += age * min<uint32_t>(f.bpm_x10, 3000) * 3579 >> 15; // 65536 turns / 600000 per ms x BPM x 10
        }
        return int(phase & 0xffff);
    }
    int tempoConfidence() const { return _controller.timeline().output().beat_confidence * 4096 / 255; }
    int bandCount() const { return _controller.timeline().output().band_count; }
    int band(int i) const { return _controller.timeline().output().bands[i]; }
    // Pitch class i (C..B) in Q12; 0 when the sender adds no chroma (EXT_CHROMA).
    int chroma(int i) const { return _controller.timeline().output().chroma[i] * 4096 / 255; }
    // Lab wave_value(): index 0 is the oldest sample; spans longer than the
    // history stretch it.
    int wave(int index, int span) const {
        constexpr int N = RemoteAudio::WAVE_HISTORY;
        if (span > N) index = index * (N - 1) / (span - 1);
        if (index > N - 1) index = N - 1;
        return _controller.timeline().wave(N - 1 - index);
    }
    uint8_t random8() { return ::random8(); }
    int16_t sin16(uint16_t t) const { return ::sin16(t); }
    uint16_t noise(uint32_t x, uint32_t y) const { return inoise16(x, y); }
    uint16_t noise(uint32_t x, uint32_t y, uint32_t z) const { return inoise16(x, y, z); }
    void put(int x, int y, uint8_t v, uint8_t hue) {
        if (_mask) _led.get_pixel(x, y).nscale8(v);
        else _led.get_pixel(x, y) = color_from_palette(_palette, uint16_t(hue) * 15, v);
    }
    // A colour of the mode's own (Fire's heat colours); the mask keeps its brightness.
    void rgb(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
        if (_mask) _led.get_pixel(x, y).nscale8(max(r, max(g, b)));
        else _led.get_pixel(x, y) = CRGB(r, g, b);
    }
};

using Engine = Viz::Engine<Host, WIDTH * HEIGHT>;

// Frame time of the last visualization frame, microseconds (status "viz_us").
inline uint32_t &lastRenderMicros() {
    static uint32_t value = 0;
    return value;
}

inline void render(Led &led, const RemoteAudioController &controller, const PaletteT &palette,
                   AudioEffectEnum mode, uint8_t v_min, uint8_t v_max) {
    static Engine engine;
    static uint32_t last_ms = 0;
    if (!handles(mode)) return;
    const uint32_t started = micros(), now = millis();
    // Long gaps (paused stream, other effect) start from a clean state.
    const uint32_t elapsed = last_ms ? now - last_ms : 0;
    if (!last_ms || elapsed > 500) engine.invalidate();
    last_ms = now;
    engine.settings = controller.overrides().tuning.viz;
    Host host(led, controller, palette, v_min, v_max);
    engine.render(host, uint8_t(mode) - FIRST, elapsed && elapsed <= 500 ? elapsed : 16);
    lastRenderMicros() = micros() - started;
}
} // namespace RemoteViz
