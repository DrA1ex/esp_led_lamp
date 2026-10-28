#pragma once
#include <stdint.h>
#include "protocol.h"

// Color-preserving modulation ported from esp_led_wasm_lab/src/render.cpp,
// experimental Audio Reaction modes 11/12. Modulation only darkens existing
// colors: it cannot create out-of-palette highlights or white pixels.
namespace RemoteReactive {
inline float clamp01(float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }
inline float absf(float v) { return v >= 0.f ? v : -v; }
inline float maximum(float a, float b) { return a > b ? a : b; }

inline uint8_t bassMidHigh(uint16_t band, uint16_t bass, uint16_t mid,
                           uint16_t high, uint16_t onset0, uint16_t onset1,
                           uint16_t onset2, bool upper, float mix = 1.f) {
    constexpr float inv = 1.f / RemoteProtocol::MAX_LEVEL;
    const float b = band * inv;
    const float main = (upper ? bass : mid) * inv;
    const float transient = (upper ? onset0 : onset1) * inv;
    const float boost = main * .40f + high * inv * .18f +
                        transient * .18f + onset2 * inv * .12f;
    const float amount = (1.f - mix) + mix * clamp01(.20f + b * .75f + boost);
    return static_cast<uint8_t>(clamp01(amount) * 255.f);
}

inline uint8_t beatReactive(uint16_t band, float pulse, uint16_t x,
                           uint16_t width, float mix = 1.f) {
    const float radius = absf(float(x) - (float(width) - 1.f) / 2.f) /
                         (float(width) / 2.f);
    const float ripple = pulse * (1.f - absf(radius - (1.f - pulse)));
    const float brightness = clamp01(.30f +
                            (float(band) / RemoteProtocol::MAX_LEVEL) * .6f +
                            ripple * .6f);
    return static_cast<uint8_t>(clamp01(1.f - mix + mix * brightness) * 255.f);
}

inline float predictedBeat(uint8_t phase, uint8_t confidence, uint16_t lowOnset) {
    // Protocol only includes phase/confidence, not the browser DSP's beatPulse.
    // Derive a short decaying pulse just after phase wraps to zero.
    const float phasePulse = clamp01(1.f - float(phase) / 80.f);
    return maximum(float(lowOnset) / RemoteProtocol::MAX_LEVEL,
                   phasePulse * (float(confidence) / 255.f));
}
} // namespace RemoteReactive
