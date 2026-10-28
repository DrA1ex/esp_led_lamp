#pragma once
#include <math.h>
#include <stdint.h>
#include "protocol.h"

// Color-preserving modulation ported from esp_led_lab/src/render.cpp,
// experimental Audio Reaction modes 11/12. Modulation only darkens existing
// colors: it cannot create out-of-palette highlights or white pixels.
// Keep these formulas in sync with the lab renderer: the browser preview is
// only meaningful when both sides compute the same per-column factors.
namespace RemoteReactive {
inline float clamp01(float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }
inline float absf(float v) { return v >= 0.f ? v : -v; }
inline float maximum(float a, float b) { return a > b ? a : b; }

// Which energy drives a row in Bass / Mid / High. A single-row strip has no
// upper/lower half, so it blends both instead of showing only one band group.
enum class Half : uint8_t { UPPER, LOWER, BOTH };

inline Half halfForRow(uint16_t y, uint16_t height) {
    if (height < 2) return Half::BOTH;
    return y < height / 2 ? Half::UPPER : Half::LOWER;
}

inline uint8_t bassMidHigh(uint16_t band, uint16_t bass, uint16_t mid,
                           uint16_t high, uint16_t onset0, uint16_t onset1,
                           uint16_t onset2, Half half, float mix = 1.f) {
    constexpr float inv = 1.f / RemoteProtocol::MAX_LEVEL;
    const float b = band * inv;
    float main, transient;
    switch (half) {
        case Half::UPPER: main = bass * inv; transient = onset0 * inv; break;
        case Half::LOWER: main = mid * inv; transient = onset1 * inv; break;
        default:
            main = (bass + mid) * .5f * inv;
            transient = maximum(onset0, onset1) * inv;
    }
    const float boost = main * .40f + high * inv * .18f +
                        transient * .18f + onset2 * inv * .12f;
    const float amount = (1.f - mix) + mix * clamp01(.20f + b * .75f + boost);
    return static_cast<uint8_t>(clamp01(amount) * 255.f);
}

inline uint8_t beatReactiveAt(uint16_t band, float pulse, float radius, float mix = 1.f) {
    const float ripple = pulse * (1.f - absf(radius - (1.f - pulse)));
    const float brightness = clamp01(.30f +
                            (float(band) / RemoteProtocol::MAX_LEVEL) * .6f +
                            ripple * .6f);
    return static_cast<uint8_t>(clamp01(1.f - mix + mix * brightness) * 255.f);
}

inline uint8_t beatReactive(uint16_t band, float pulse, uint16_t x,
                           uint16_t width, float mix = 1.f) {
    return beatReactiveAt(band, pulse, absf(float(x) - (float(width) - 1.f) / 2.f) / (float(width) / 2.f), mix);
}

// Browser DSP: beat_pulse jumps to 1 when the phase wraps and decays to zero
// within 230 ms. The wire format has phase and BPM, so reconstruct the same
// envelope from the elapsed fraction of the current beat period.
inline float beatPulse(uint8_t phase, uint16_t bpm_x10) {
    if (bpm_x10 == 0) return clamp01(1.f - float(phase) / 80.f);
    const float since_beat_s = float(phase) / 256.f * 600.f / float(bpm_x10);
    return clamp01(1.f - since_beat_s / .23f);
}

inline float predictedBeat(uint8_t phase, uint8_t confidence, uint16_t lowOnset,
                           uint16_t bpm_x10 = 0) {
    return maximum(float(lowOnset) / RemoteProtocol::MAX_LEVEL,
                   beatPulse(phase, bpm_x10) * (float(confidence) / 255.f));
}

// Origin shared by every spatial audio mode (Wave, Spectrum, Line, their
// Center variants, Bass / Mid / High, Beat-aware). Each column x is evaluated
// at a mapped coordinate x' (see OriginMap::x): normal modes start at the
// shifted column, Center modes are centred at WIDTH / 2 + shift. `reverse`
// runs the effect from the other end. With `ring` the coordinate wraps
// (closed strips), otherwise LEDs shifted past the end stay dark. `radial`
// makes Center modes and Beat-aware use the 2-D distance from
// (WIDTH / 2, center_y) on matrices. The default is the stock geometry.
// Mirrored by origin_x()/origin_*() in the lab renderer (render.cpp).
struct Origin {
    int16_t shift = 0;       // permille of WIDTH, -500..500
    uint16_t center_y = 500; // permille of HEIGHT - 1 (radial only)
    bool reverse = false;
    bool ring = false;
    bool radial = false;

    bool is_default() const { return shift == 0 && center_y == 500 && !reverse && !ring && !radial; }
};

class OriginMap {
    int _width, _height, _shift;
    float _row;
    Origin _origin;
public:
    OriginMap(uint16_t width, uint16_t height, const Origin &origin) :
            _width(width ? width : 1), _height(height ? height : 1), _origin(origin) {
        // Round half away from zero so +/- shifts are symmetric.
        const int32_t scaled = int32_t(_width) * origin.shift;
        _shift = (scaled >= 0 ? scaled + 500 : scaled - 500) / 1000;
        _row = float(_height - 1) * (origin.center_y > 1000 ? 1000 : origin.center_y) / 1000.f;
    }
    int width() const { return _width; }
    // Mapped column, or -1 when it falls outside a non-ring strip.
    int x(int column) const {
        int v = (_origin.reverse ? _width - 1 - column : column) - _shift;
        if (_origin.ring) { v %= _width; if (v < 0) v += _width; return v; }
        return v >= 0 && v < _width ? v : -1;
    }
    bool radial() const { return _origin.radial && _height > 1; }
    // Distance of mapped column xp (row y) from the centre WIDTH / 2.
    float distance(int xp, int y) const {
        const float dx = absf(float(xp - _width / 2));
        if (!radial()) return dx;
        const float dy = float(y) - _row;
        return sqrtf(dx * dx + dy * dy);
    }
    float maxDistance() const {
        const int half = _width / 2;
        const float mx = float(half > _width - 1 - half ? half : _width - 1 - half);
        if (!radial()) return mx < 1.f ? 1.f : mx;
        const float my = _row > _height - 1 - _row ? _row : _height - 1 - _row;
        const float d = sqrtf(mx * mx + my * my);
        return d < 1.f ? 1.f : d;
    }
    // Wave / Spectrum Center provider index (0..WIDTH / 2); the stock
    // |x - WIDTH / 2| by default, radial distances rescaled to the same range.
    int centeredIndex(int xp, int y) const {
        if (!radial()) return xp <= _width / 2 ? _width / 2 - xp : xp - _width / 2;
        const int index = int(distance(xp, y) * (_width / 2) / maxDistance() + .5f);
        return index < _width / 2 ? index : _width / 2;
    }
    // Line Center: is (xp, y) lit when the line reached `reach` of WIDTH / 2?
    // Non-radial reproduces AudioEffectManager::line_centered exactly.
    bool lineCenteredLit(int xp, int y, int reach) const {
        if (!radial()) return (xp < _width / 2 ? _width / 2 - xp : xp - (_width - 1 - _width / 2)) <= reach;
        return distance(xp, y) * (_width / 2) / maxDistance() < float(reach);
    }
    // Beat-aware radius 0..1: stock |x - (W - 1) / 2| / (W / 2) unless radial.
    float beatRadius(int xp, int y) const {
        if (!radial()) return absf(float(xp) - float(_width - 1) / 2.f) / (float(_width) / 2.f);
        return distance(xp, y) / maxDistance();
    }
};

// FastLED dim8_raw (scale8 with FASTLED_SCALE8_FIXED), host-testable.
inline uint8_t dim8Raw(uint8_t v) { return uint8_t((uint16_t(v) * (uint16_t(v) + 1)) >> 8); }

// Lab "Spectrum expression" + "Spectrum onset": blends the stock squared
// dim8_raw curve towards a brighter perceptual one and adds onset accents.
// shape == 0 and accent == 0 reproduce the stock AudioEffectManager factor.
inline uint8_t shapedLevel(uint8_t value, float shape, float accent) {
    const float norm = value / 255.f;
    const float brightened = .25f * norm + .75f * sqrtf(norm);
    const float shaped = (1.f - shape) * dim8Raw(value) / 255.f + shape * brightened + accent;
    return static_cast<uint8_t>(clamp01(shaped) * 255.f);
}

// Lab spectrum onset groups use the column (not band) index, like render.cpp.
inline uint8_t onsetGroup(uint16_t index, uint16_t width) {
    const uint32_t g = uint32_t(index) * 16 / (width ? width : 1);
    return g < 5 ? 0 : (g < 12 ? 1 : 2);
}

// Control sources for Brightness / Line / Speed / Scale / Light. Same order as
// the lab's "Signal source" selector, transmitted as config.param_source.
enum class ControlSource : uint8_t { LOUDNESS, SPECTRAL, BASS, MID, TREBLE, PEAK, ONSET, BEAT, COUNT };

inline uint16_t controlValue(const RemoteProtocol::Frame &f, const uint16_t *pulses, ControlSource source) {
    constexpr uint16_t MAX = RemoteProtocol::MAX_LEVEL;
    switch (source) {
        case ControlSource::SPECTRAL: {
            // Lab new_param_value(): 0.45 * strongest band + 0.55 * mean, x1.55.
            uint32_t sum = 0;
            uint16_t peak = 0;
            for (uint8_t i = 0; i < RemoteProtocol::BAND_COUNT; ++i) {
                sum += f.bands[i];
                if (f.bands[i] > peak) peak = f.bands[i];
            }
            const float mean = float(sum) / RemoteProtocol::BAND_COUNT;
            const float v = (.45f * peak + .55f * mean) * 1.55f;
            return v > MAX ? MAX : uint16_t(v);
        }
        case ControlSource::BASS: return f.bass;
        case ControlSource::MID: return f.mid;
        case ControlSource::TREBLE: return f.high;
        case ControlSource::PEAK: return f.peak;
        case ControlSource::ONSET: {
            uint16_t v = pulses[0];
            if (pulses[1] > v) v = pulses[1];
            if (pulses[2] > v) v = pulses[2];
            return v;
        }
        case ControlSource::BEAT:
            return uint16_t(beatPulse(f.beat_phase, f.bpm_x10) * (f.beat_confidence / 255.f) * MAX);
        default: return f.rms;
    }
}
} // namespace RemoteReactive
