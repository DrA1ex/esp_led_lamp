#pragma once
// Geometry-aware audio visualizations shared by the ESP LED Music Lab
// (esp_led_lab/src/viz_engine.h, WebAssembly preview) and the firmware
// (esp_led_lamp/src/remote_audio/viz_engine.h). Keep both copies identical.
//
// Integer only: the ESP8266 has no FPU and soft-float costs hundreds of
// cycles per operation. Levels are Q12 (0..4096 = 0..1, like the wire
// protocol), time is in ms, 2-D positions and distances are Q4 (1/16 LED),
// phases are Q16 turns (65536 = one turn).
//
// Freestanding: no libc. The host supplies the inputs, the reaction
// transfer, random numbers, sin16, noise and the pixel output:
//   int width(), height();  int originX(int x) (-1: dark);
//   bool radial(); int radialRow() (row of the radial centre, Q8);
//   int transfer(int v) (reaction range / window / curve, Q12 -> Q12);
//   int control(), lineSource(), level(int g), pulse(int g);  (Q12)
//   int beat();  (predicted beat pulse x tempo confidence, Q12)
//   int beatPhase();  (Q16 turn since the last predicted beat)
//   int tempoConfidence();  (Q12)
//   int bandCount(); int band(int i); int wave(int index, int span);  (Q12)
//   int chroma(int i);  (pitch class i = C, C#, ... B; Q12, 4096 = the strongest one)
//   uint8_t random8(); int16_t sin16(uint16_t);
//   uint16_t noise(uint32_t x, uint32_t y[, uint32_t z]);  (inoise16)
//   void put(int x, int y, uint8_t v, uint8_t hue);  (palette colour, brightness)
//   void rgb(int x, int y, uint8_t r, uint8_t g, uint8_t b);  (a colour of its own)
//
// Strips (HEIGHT < 4, every row the same) run along the LEDs from the origin,
// or out from the centre when mirrored; matrices use the height and mirror
// vertically. y = 0 is the bottom row.
//
// Every mode can take up to MAX_PARAMS parameters (0..255 each, the "params"
// message): paramDefaults() / paramCount() below, their meaning next to each
// mode, their names in the lab's web/js/fx-params.js. From Particles on, the
// last parameter is the mode's brightness (128 = x1, 255 = x2).
#include <stdint.h>

#ifndef VIZ_WITH_HEAVY
#define VIZ_WITH_HEAVY 1
#endif

namespace Viz {
// Lab audio mode = 13 + id, firmware AudioEffectEnum = 12 + id.
enum Mode : uint8_t {
    VU_METER, SPECTRUM_BARS, FLOW, PULSES, SPARKLES, MOOD, WAVEFORM, BAND_METERS,
    FIRE, AURORA, JUGGLE, RADIAL_SPECTRUM, BLOBS,
    // Constructors: their parameters pick the shape, the trigger, the motion and the colours.
    PARTICLES, FIELD, ZONES,
    // Follows the dominant frequency: the one mode that needs the spectrum's shape, not its energy.
    MELODY,
    // Moves with the beat phase, not with single onsets.
    GROOVE,
    // Follows the pitch classes (chroma): which notes sound, not how loud.
    CHROMATIC_FLOW, MODE_COUNT
};
// Per-pixel noise, angles and fields: compiled only with VIZ_WITH_HEAVY.
inline bool heavy(uint8_t m) { return m == FIRE || m == AURORA || m == RADIAL_SPECTRUM || m == BLOBS || m == FIELD || m == CHROMATIC_FLOW; }
inline bool available(uint8_t m) { return m < MODE_COUNT && (VIZ_WITH_HEAVY || !heavy(m)); }

constexpr int32_t ONE = 4096; // Q12 1.0

// Effect parameters. PARAMS_VERSION changes whenever a parameter's meaning or
// order changes, so a sender never drives a lamp with a different layout.
constexpr uint8_t PARAMS_VERSION = 1;
constexpr int MAX_PARAMS = 11;
inline uint8_t paramCount(uint8_t m) {
    static constexpr uint8_t count[MODE_COUNT] = {2, 2, 0, 3, 4, 4, 1, 0, 8, 5, 2, 2, 3, 10, 11, 9, 9, 8, 10};
    return m < MODE_COUNT ? count[m] : 0;
}
// Defaults: the former constants of the existing modes (Fire has a new look);
// the constructors start from their spectral variants. Brightness 128 = x1.
// The brightness parameter of mode m, -1 for a mode without one.
inline int brightnessParam(uint8_t m) { return m >= PARTICLES && m < MODE_COUNT ? paramCount(m) - 1 : -1; }
inline const uint8_t *paramDefaults(uint8_t m) {
    static constexpr uint8_t table[MODE_COUNT + 1][MAX_PARAMS] = {
        {0, 1},                                     // VU meter: colours, peak dot
        {0, 1},                                     // Spectrum bars: colours, peaks
        {0},                                        // Flow
        {0, 24, 31},                                // Pulses: trigger, width, glow
        {64, 13, 89, 0},                            // Sparkles: density, hit burst, bass glow, colours
        {48, 90, 200, 0},                           // Mood: spread, flash, tone range, map
        {0},                                        // Waveform: colours
        {0},                                        // Band meters
        {0, 170, 140, 140, 128, 140, 0, 60},        // Fire: fuel, height, intensity, flares, wind, blend, colours, embers
        {64, 128, 90, 77, 0},                       // Aurora: zoom, fullness, colour spread, flash, map
        {8, 128},                                   // Juggle: dots, size
        {128, 90},                                  // Radial spectrum: spin, core
        {128, 4, 127},                              // Blobs: size, count, softness
        {0, 0, 100, 100, 170, 90, 110, 90, 0, 128},      // Particles: shape, trigger, amount, speed, gravity, spread, life, trail, colours, brightness
        {0, 100, 80, 1, 1, 120, 100, 60, 120, 30, 128},  // Field: pattern, zoom, flow, map, drive, pump, kick, contrast, colours, drift, brightness
        {8, 0, 0, 1, 100, 40, 0, 60, 128},               // Zones: zones, layout, trigger, pattern, decay, glow, colours, edge, brightness
        {40, 200, 0, 80, 100, 120, 0, 40, 128},          // Melody: from, to, style, glide, size, trail, colours, gate, brightness
        {0, 64, 200, 160, 100, 100, 0, 128},             // Groove: shape, step, snap, pump, zoom, sparks, colours, brightness
        {0, 0, 100, 80, 80, 60, 128, 90, 60, 128},       // Chromatic Flow: order, shift, follow, focus, zoom, flow, blend, glow, swell, brightness
        {0},
    };
    return table[m < MODE_COUNT ? int(m) : int(MODE_COUNT)];
}

struct Settings {
    bool mirror = false;    // strips: from the centre both ways; matrices: from the middle row
    bool autolevel = true;  // each band / meter stretched over its own recent range
    uint8_t decay = 50;     // 0..100 %: meter release and trail length
    uint8_t speed = 50;     // 0..100 %: motion speed (Flow, Pulses and the free-form modes)
    // Effect parameters for mode `paramsMode`; every other mode runs with its defaults.
    uint8_t paramsMode = 0xff;
    uint8_t params[MAX_PARAMS] = {};
};

namespace detail {
inline int32_t mini(int32_t a, int32_t b) { return a < b ? a : b; }
inline int32_t maxi(int32_t a, int32_t b) { return a > b ? a : b; }
inline int32_t clampi(int32_t v, int32_t l, int32_t h) { return v < l ? l : (v > h ? h : v); }
inline int32_t absi(int32_t v) { return v < 0 ? -v : v; }
inline uint8_t u8(int32_t a) { return uint8_t(clampi(a, 0, 255)); }
// Q12 -> 0..255.
inline uint8_t u8q(int32_t q) { return u8((q * 255) >> 12); }
inline uint32_t isqrt(uint32_t v) {
    uint32_t r = 0, bit = 1u << 30;
    while (bit > v) bit >>= 2;
    while (bit) {
        if (v >= r + bit) { v -= r + bit; r = (r >> 1) + bit; }
        else r >>= 1;
        bit >>= 2;
    }
    return r;
}
inline int32_t dist(int32_t dx, int32_t dy) { return int32_t(isqrt(uint32_t(dx * dx) + uint32_t(dy * dy))); }
// Angle of (x, y) in Q16 turns: atan(t) / 2pi ~ t (0.125 + 0.0435 (1 - t)) in
// the first octant (error < 0.0006 turn), then mirrored into place.
inline uint16_t angle16(int32_t y, int32_t x) {
    const uint32_t ax = uint32_t(absi(x)), ay = uint32_t(absi(y));
    if (!ax && !ay) return 0;
    const bool steep = ay > ax;
    const uint32_t t = ((steep ? ax : ay) << 15) / (steep ? ay : ax); // Q15, 0..1
    uint32_t a = (t * (16384 + ((5695 * (32768 - t)) >> 15))) >> 16;
    if (steep) a = 16384 - a;
    if (x < 0) a = 32768 - a;
    if (y < 0) a = 65536 - a;
    return uint16_t(a);
}
// FastLED HeatColor: black -> red -> yellow -> white as the temperature rises.
inline void heatColor(uint8_t temperature, uint8_t &r, uint8_t &g, uint8_t &b) {
    const uint8_t t192 = uint8_t(((temperature * 191) >> 8) + (temperature ? 1 : 0)); // scale8_video(t, 191)
    const uint8_t ramp = uint8_t((t192 & 63) << 2);
    if (t192 & 128) { r = 255; g = 255; b = ramp; }
    else if (t192 & 64) { r = 255; g = ramp; b = 0; }
    else { r = ramp; g = 0; b = 0; }
}
} // namespace detail

template<class Host, int MAX_PIX>
class Engine {
    static constexpr int MAX_BANDS = 64, CHANNELS = MAX_BANDS + 1, OVERALL = MAX_BANDS;
    static constexpr int SPARKS = 32, ZONES_MAX = 16;
    struct Comet { int16_t pos, energy; uint8_t hue; bool on; }; // pos, energy Q12
    // Particle: position Q16 of the lamp (x across a matrix, y along the main
    // axis: up a matrix, along a strip), velocity Q12 lamps per second, life Q12.
    struct Spark { int32_t x, y; int16_t vx, vy, life; uint8_t hue, on; };
    struct Zone { int16_t level; uint8_t hue; };
    // Groove: the beat clock (beats, Q16: the phase is its low half) and its
    // rate (Q16 beats per second), the pattern's travel (Q16 periods, wraps),
    // the bass expansion (Q12), the direction and a turn due at the next beat.
    struct Groove { uint32_t pos, travel; int32_t rate; int16_t boost; int8_t dir; uint8_t turn, locked, hue; };

    int16_t _level[CHANNELS], _peak[CHANNELS], _hold[CHANNELS]; // Q12, Q12, ms
    int32_t _lo[CHANNELS], _hi[CHANNELS];                       // auto-level range, Q20
    uint8_t _v[MAX_PIX], _h[MAX_PIX];
    int16_t _prev_pulse[3], _cool[4], _tone = 0, _mood = 0, _grow[4], _prev_beat = 0, _boost = 0;
    int32_t _clock = 0, _spawn = 0; // ms, Q8 sparks
    uint16_t _phase[8], _phase2[8]; // Q16 turns
    uint32_t _z = 0;                // Aurora / Field time, noise units
    // Only one mode runs at a time: Pulses, Particles, Zones and Groove share this memory.
    union { Comet _comets[24]; Spark _sparks[SPARKS]; Zone _zones[ZONES_MAX]; Groove _groove; };
    uint8_t _cursor = 0, _count = 0, _hit_hue = 0; // Zones: chase position, built zones, colour per hit
    int8_t _dir = 1;
    uint8_t _p[MAX_PARAMS] = {};
    int32_t _key = -1;
    int32_t _gain = 256; // brightness of the running mode, Q8
    Host *_host = nullptr;
    int W = 1, H = 1, _dt = 16;

public:
    Settings settings;

    void invalidate() { _key = -1; }

    // dt: milliseconds since the previous frame.
    void render(Host &host, uint8_t mode, uint16_t dt) {
        _host = &host; W = host.width(); H = host.height(); _dt = detail::clampi(dt, 1, 100);
        if (!available(mode) || W < 1 || H < 1 || W * H > MAX_PIX) return;
        const int32_t key = int32_t(mode) * 100000 + W * 128 + H + (settings.mirror ? 50000 : 0);
        if (key != _key) { _key = key; reset(mode); }
        const uint8_t *p = settings.paramsMode == mode ? settings.params : paramDefaults(mode);
        for (int i = 0; i < MAX_PARAMS; i++) _p[i] = p[i];
        _gain = brightnessParam(mode) >= 0 ? 2 * _p[brightnessParam(mode)] : 256;
        _tone = int16_t(approach(_tone, toneNow(), 150, 150));
        switch (mode) {
            case VU_METER: vuMeter(); break;
            case SPECTRUM_BARS: spectrumBars(); break;
            case FLOW: flow(); break;
            case PULSES: pulses(); break;
            case SPARKLES: sparkles(); break;
            case MOOD: mood(); break;
            case WAVEFORM: waveform(); break;
            case BAND_METERS: bandMeters(); break;
            case JUGGLE: juggle(); break;
            case PARTICLES: particles(); break;
            case ZONES: zones(); break;
            case MELODY: melody(); break;
            case GROOVE: groove(); break;
#if VIZ_WITH_HEAVY
            case FIRE: fire(); break;
            case AURORA: aurora(); break;
            case RADIAL_SPECTRUM: radialSpectrum(); break;
            case BLOBS: blobs(); break;
            case FIELD: field(); break;
            case CHROMATIC_FLOW: chromaticFlow(); break;
#endif
            default: break;
        }
    }

private:
    // ---- shared helpers -------------------------------------------------
    void reset(uint8_t mode) {
        for (int i = 0; i < CHANNELS; i++) { _level[i] = _peak[i] = _hold[i] = 0; _lo[i] = _hi[i] = 0; }
        for (int i = 0; i < MAX_PIX; i++) _v[i] = _h[i] = 0;
        if (mode == PARTICLES) for (Spark &s: _sparks) s.on = 0;
        else if (mode == ZONES) for (Zone &z: _zones) z = Zone{0, 0};
        else if (mode == GROOVE) _groove = Groove{0, 0, 131072, 0, 1, 0, 0, 0}; // 120 BPM until it hears one
        else for (Comet &c: _comets) c.on = false;
        for (int g = 0; g < 3; g++) _prev_pulse[g] = 0;
        for (int k = 0; k < 4; k++) _cool[k] = _grow[k] = 0;
        // Start phases k * 0.13 turn; the second axis runs 1.37x (Juggle) or 1.31x (Blobs) faster.
        for (int k = 0; k < 8; k++) { _phase[k] = uint16_t(k * 8520); _phase2[k] = uint16_t(k * (mode == BLOBS ? 11161 : 11672)); }
        _clock = _spawn = 0; _mood = 0; _z = 0; _prev_beat = _boost = 0;
        _cursor = 255; _count = 0; _hit_hue = 0; _dir = 1;
        _tone = int16_t(toneNow());
    }
    // Parameter i of the running mode (0..255).
    int P(int i) const { return _p[i]; }
    bool matrix() const { return H >= 4; }
    int bands() const { const int n = _host->bandCount(); return n > 0 && n <= MAX_BANDS ? n : 16; }
    // Cell i of `cells` from n per-band values: fewer cells than bands average
    // their band range, more cells interpolate (firmware RemoteSignalProvider).
    template<class F>
    static int32_t cellOf(F get, int n, int i, int cells) {
        using namespace detail;
        if (cells <= 1) { int32_t sum = 0; for (int k = 0; k < n; k++) sum += get(k); return sum / n; }
        if (cells < n) {
            const int a = i * n / cells, b = maxi(a + 1, (i + 1) * n / cells);
            int32_t sum = 0; for (int k = a; k < b; k++) sum += get(k);
            return sum / (b - a);
        }
        const int32_t pos = int32_t(i) * (n - 1) * 256 / (cells - 1);
        const int a = pos >> 8;
        if (a >= n - 1) return get(n - 1);
        const int32_t f = pos & 255;
        return (get(a) * (256 - f) + get(a + 1) * f) >> 8;
    }
    int32_t bandCell(int i, int cells) const { return cellOf([&](int k) { return int32_t(_host->band(k)); }, bands(), i, cells); }
    // 0 = bass-heavy .. ONE = treble-heavy.
    int32_t toneNow() const {
        const int32_t b = _host->level(0), m = _host->level(1), h = _host->level(2), sum = b + m + h;
        return sum > 82 ? ((m >> 1) + h) * ONE / sum : _tone;
    }
    int32_t approach(int32_t cur, int32_t target, int32_t attack, int32_t release) const {
        const int32_t tau = detail::maxi(1, target > cur ? attack : release);
        return _dt >= tau ? target : cur + (target - cur) * _dt / tau;
    }
    int32_t releaseMs() const { return 40 + 7 * settings.decay; }
    int32_t level01(int32_t v) const { return detail::clampi(_host->transfer(detail::clampi(v, 0, ONE)), 0, ONE); }
    // Auto-level channel i: its own recent range (~3 s) is stretched towards
    // 0..1, so quiet bands and the treble meter move like the bass. Only part
    // of the floor is subtracted, so a steady signal stays up instead of 0.
    int32_t autolevel(int i, int32_t x, int32_t dt) {
        using namespace detail;
        if (!settings.autolevel) return x;
        const int32_t xs = x << 8;
        _hi[i] = xs > _hi[i] ? xs : _hi[i] + (xs - _hi[i]) * dt / 3000;
        _lo[i] = xs < _lo[i] ? xs : _lo[i] + (xs - _lo[i]) * dt / 3000;
        const int32_t floor = _lo[i] * 4 / 5, num = xs - floor, den = maxi(_hi[i] - floor, 410 << 8);
        return num <= 0 ? 0 : mini(ONE, ((num >> 4) << 12) / (den >> 4));
    }
    int32_t overall(int32_t line) { return level01(autolevel(OVERALL, line, _dt)); }
    uint8_t random8() { return _host->random8(); }
    int random(int n) { return ((random8() << 8) | random8()) % detail::maxi(1, n); }
    // sin of a Q16 phase shifted to 0..65535.
    int32_t wave16(uint16_t phase) const { return int32_t(_host->sin16(phase)) + 32768; }
    // Phase advanced by `rate` Q16 turns per second.
    uint16_t advance(uint16_t phase, int32_t rate) const { return uint16_t(phase + rate * _dt / 1000); }

    int stripCells() const { return settings.mirror ? W / 2 + 1 : W; }
    int stripCell(int x) const {
        const int xp = _host->originX(x);
        if (xp < 0) return -1;
        return settings.mirror ? (xp <= W / 2 ? W / 2 - xp : xp - W / 2) : xp;
    }
    int riseCells(bool mirror) const { return mirror ? (H + 1) / 2 : H; }
    int riseCell(int y, bool mirror) const { if (!mirror) return y; const int c = H / 2; return y < c ? c - 1 - y : y - c; }
    bool radial() const { return _host->radial() && H > 1; }
    int32_t row16() const { return _host->radialRow() >> 4; }
    int32_t originMaxDistance16() const {
        using namespace detail;
        const int32_t mx = maxi(W / 2, W - 1 - W / 2) * 16;
        if (!radial()) return maxi(16, mx);
        const int32_t cy = row16(), my = maxi(cy, (H - 1) * 16 - cy);
        return maxi(16, dist(mx, my));
    }
    void put(int x, int y, uint8_t v, int32_t hue) { _host->put(x, y, _gain == 256 ? v : detail::u8((v * _gain) >> 8), detail::u8(hue)); }
    void stripPut(int x, uint8_t v, int32_t hue) { for (int y = 0; y < H; y++) put(x, y, v, hue); }

    void meter(int i, int32_t target) {
        using namespace detail;
        _level[i] = int16_t(approach(_level[i], target, 15, releaseMs()));
        if (_level[i] >= _peak[i]) { _peak[i] = _level[i]; _hold[i] = 400; }
        else if (_hold[i] > 0) _hold[i] = int16_t(_hold[i] - _dt);
        else _peak[i] = int16_t(maxi(_level[i], _peak[i] - _dt * (102400 + 6144 * (100 - settings.decay)) / 100000));
    }
    // Fill of `cells` up to level with an anti-aliased edge, plus the peak dot.
    static uint8_t meterV(int cell, int cells, int32_t level, int32_t peak) {
        using namespace detail;
        const int32_t edge = level * cells;
        uint8_t v = u8((edge - (int32_t(cell) << 12)) >> 4);
        const int pk = mini(cells - 1, (peak * cells) >> 12);
        if (peak > 82 && cell == pk && pk >= (edge >> 12)) v = 255;
        return v;
    }
    // Rising-edge onsets per low / mid / high group. Each group re-triggers
    // after 400 / 320 / 240 ms and onsets are at least 150 ms apart.
    int onsets(int32_t strength[3]) {
        int mask = 0;
        for (int k = 0; k < 4; k++) _cool[k] = int16_t(detail::maxi(-1000, _cool[k] - _dt));
        for (int g = 0; g < 3; g++) {
            const int32_t p = _host->pulse(g);
            if (p > 2048 && p > _prev_pulse[g] + 819 && _cool[g] <= 0 && _cool[3] <= 0) {
                mask |= 1 << g; strength[g] = p; _cool[g] = int16_t(400 - 80 * g); _cool[3] = 150;
            }
            _prev_pulse[g] = int16_t(p);
        }
        return mask;
    }
    // onsets() plus predicted beats as bit 3: the beat pulse jumping above 0.4,
    // which needs a confident tempo. Call once per frame.
    int hits(int32_t strength[4]) {
        const int mask = onsets(strength);
        const int32_t b = _host->beat();
        const bool beat = b > 1638 && b > _prev_beat + 819;
        _prev_beat = int16_t(b);
        strength[3] = beat ? b : 0;
        return mask | (beat ? 8 : 0);
    }
    // A trigger parameter: 0..2 hits of bass / mid / treble, 3 hits of any
    // group, 4 beats. Returns the hit strength (0: none this frame) and its group (3: beat).
    static int32_t fired(int trigger, int mask, const int32_t strength[4], int &group) {
        if (trigger == 4) { group = 3; return mask & 8 ? strength[3] : 0; }
        for (int g = 0; g < 3; g++)
            if ((trigger == 3 || trigger == g) && (mask & (1 << g))) { group = g; return strength[g]; }
        return 0;
    }
    static int32_t groupHue(int g) { return g < 3 ? g * 85 + 20 : 20; }
    // Band i of n as a palette position, on the same scale as groupHue (bass 20 .. treble 190).
    static int32_t bandHue(int i, int n) { return 20 + i * 170 / detail::maxi(1, n - 1); }
    // Per-band onsets: a band jumping well above its own recent average (~0.3 s)
    // calls hit(band, count, strength Q12); each band then rests 150 ms. Keeps
    // the average in _peak and the rest in _hold, which only the meter modes use.
    template<class F>
    void bandHits(F hit) {
        using namespace detail;
        const int n = bands();
        for (int i = 0; i < n; i++) {
            const int32_t raw = _host->band(i), avg = _peak[i];
            if (_hold[i] > 0) _hold[i] = int16_t(_hold[i] - _dt);
            if (raw > 819 && raw > avg + maxi(410, avg >> 1) && _hold[i] <= 0) { _hold[i] = 150; hit(i, n, level01(raw)); }
            _peak[i] = int16_t(approach(avg, raw, 300, 300));
        }
    }
    // Auto-levelled band levels and their value at spectrum position t (Q12, 0 = lowest band).
    int spectrum(int32_t lv[MAX_BANDS]) {
        const int n = bands();
        for (int i = 0; i < n; i++) lv[i] = level01(autolevel(i, _host->band(i), _dt));
        return n;
    }
    static int32_t spectrumAt(const int32_t lv[], int n, int32_t t) {
        const int32_t pos = detail::clampi(t, 0, ONE) * (n - 1), a = pos >> 12, f = pos & 4095;
        return a >= n - 1 ? lv[n - 1] : (lv[a] * (ONE - f) + lv[a + 1] * f) >> 12;
    }
    void fade(int n, int32_t tau) {
        using namespace detail;
        const uint32_t keep = uint32_t(255 - mini(255, 255 * _dt / maxi(1, tau) + 1));
        for (int i = 0; i < n; i++) { const uint32_t x = _v[i] * keep; _v[i] = uint8_t((x + 1 + (x >> 8)) >> 8); }
    }
    void stamp(int i, uint8_t v, int32_t hue) { if (v >= _v[i]) { _v[i] = v; _h[i] = detail::u8(hue); } }

    // ---- modes -------------------------------------------------------------
    // VU meter: strip fill from the origin (or both ways), matrix fill from the
    // bottom (or the middle, or a circle with the radial origin).
    // Parameters: colours (0 along the meter, 1 by level, 2 by tone), peak dot (0 off).
    void vuMeter() {
        using namespace detail;
        meter(0, level01(autolevel(0, _host->lineSource(), _dt)));
        const int colors = P(0) % 3;
        const int32_t peak = P(1) ? _peak[0] : 0;
        auto hue = [&](int c, int cells) -> int32_t {
            return colors == 1 ? (_level[0] * 230) >> 12 : colors == 2 ? (_tone * 255) >> 12 : c * 230 / maxi(1, cells - 1);
        };
        if (!matrix()) {
            const int cells = stripCells();
            for (int x = 0; x < W; x++) {
                const int c = stripCell(x);
                stripPut(x, c < 0 ? 0 : meterV(c, cells, _level[0], peak), hue(c, cells));
            }
            return;
        }
        const bool round = radial();
        const int cells = round ? maxi(W, H) / 2 + 1 : riseCells(settings.mirror);
        const int32_t maxd = originMaxDistance16(), cy = row16();
        for (int x = 0; x < W; x++) {
            const int xp = _host->originX(x);
            for (int y = 0; y < H; y++) {
                if (xp < 0) { put(x, y, 0, 0); continue; }
                const int c = round ? mini(cells - 1, dist(xp * 16 - W * 8, y * 16 - cy) * (cells - 1) / maxd)
                                    : riseCell(y, settings.mirror);
                put(x, y, meterV(c, cells, _level[0], peak), hue(c, cells));
            }
        }
    }
    // Spectrum: strip = smooth bands along the LEDs, matrix = bars with
    // falling peaks. Meters run per band and are spread over the cells.
    // Parameters: colours (0 by height / along, 1 by band, 2 by level), peaks (0 off).
    void spectrumBars() {
        using namespace detail;
        const int n = bands(), colors = P(0) % 3;
        for (int i = 0; i < n; i++) meter(i, level01(autolevel(i, _host->band(i), _dt)));
        auto level = [&](int c, int cells) { return cellOf([&](int k) { return int32_t(_level[k]); }, n, c, cells); };
        if (!matrix()) {
            const int cells = stripCells();
            for (int x = 0; x < W; x++) {
                const int c = stripCell(x);
                const int32_t l = c < 0 ? 0 : level(c, cells);
                stripPut(x, u8q(l), colors == 2 ? (l * 255) >> 12 : c * 255 / maxi(1, cells - 1));
            }
            return;
        }
        const int rows = riseCells(settings.mirror);
        for (int x = 0; x < W; x++) {
            const int xp = _host->originX(x);
            const int32_t l = xp < 0 ? 0 : level(xp, W);
            const int32_t p = xp < 0 || !P(1) ? 0 : cellOf([&](int k) { return int32_t(_peak[k]); }, n, xp, W);
            for (int y = 0; y < H; y++) {
                if (xp < 0) { put(x, y, 0, 0); continue; }
                const int r = riseCell(y, settings.mirror);
                const int32_t hue = colors == 1 ? xp * 255 / maxi(1, W - 1) : colors == 2 ? (l * 255) >> 12 : r * 255 / maxi(1, rows - 1);
                put(x, y, meterV(r, rows, l, p), hue);
            }
        }
    }
    // Flow: strip = loudness river coloured by tone, matrix = scrolling spectrogram.
    void flow() {
        using namespace detail;
        const bool m = matrix();
        const int32_t step = 90 - 78 * settings.speed / 100;
        const int len = m ? riseCells(settings.mirror) : stripCells(), cols = m ? W : 1, n = bands();
        _clock += _dt;
        for (int k = 0; _clock >= step && k < 8; k++) {
            _clock -= step;
            for (int a = len - 1; a > 0; a--)
                for (int c = 0; c < cols; c++) { _v[a * cols + c] = _v[(a - 1) * cols + c]; _h[a * cols + c] = _h[(a - 1) * cols + c]; }
            if (m) {
                int32_t lv[MAX_BANDS];
                for (int i = 0; i < n; i++) lv[i] = level01(autolevel(i, _host->band(i), step));
                for (int c = 0; c < cols; c++) _v[c] = _h[c] = u8q(cellOf([&](int i) { return lv[i]; }, n, c, cols));
            } else {
                _v[0] = u8q(level01(autolevel(0, _host->lineSource(), step)));
                _h[0] = u8q(_tone);
            }
        }
        if (!m) {
            for (int x = 0; x < W; x++) { const int c = stripCell(x); stripPut(x, c < 0 ? 0 : _v[c], c < 0 ? 0 : _h[c]); }
            return;
        }
        for (int x = 0; x < W; x++) {
            const int xp = _host->originX(x);
            for (int y = 0; y < H; y++) {
                if (xp < 0) { put(x, y, 0, 0); continue; }
                // Newest row at the top (or in the middle when mirrored), older rows move away.
                const int age = settings.mirror ? riseCell(y, true) : H - 1 - y, i = age * cols + xp;
                put(x, y, _v[i], _h[i]);
            }
        }
    }
    // Pulses: onsets launch comets along a strip, rings from the centre on a matrix.
    // Parameters: trigger (0 every group, 1 bass, 2 mid, 3 treble, 4 beats),
    // ring width (1/16 LED), glow with the level.
    void pulses() {
        using namespace detail;
        const bool m = matrix();
        const int32_t line = level01(_host->lineSource());
        int32_t strength[4] = {0, 0, 0, 0};
        const int mask = hits(strength), trigger = P(0) % 5;
        for (int g = 0; g < 4; g++)
            if ((mask & (1 << g)) && (trigger == 0 ? g < 3 : trigger == 4 ? g == 3 : g == trigger - 1)) {
                Comet *slot = &_comets[0];
                for (Comet &c: _comets) { if (!c.on) { slot = &c; break; } if (c.energy < slot->energy) slot = &c; }
                *slot = {0, int16_t(mini(ONE, (strength[g] * (2253 + ((1843 * line) >> 12))) >> 12)), uint8_t(groupHue(g)), true};
            }
        const int n = m ? W * H : stripCells();
        fade(n, releaseMs() * 3 / 10);
        // Rings: radius pos (ONE = half the diagonal), `width` / 16 LEDs thick.
        const int32_t width = maxi(4, P(1));
        const bool round = m && radial();
        const int32_t rmax = m ? maxi(16, dist(W * 16, H * 16) / 2) : 16;
        const int32_t cx = round ? W * 8 : (W - 1) * 8, cy = round ? row16() : (H - 1) * 8;
        const int32_t speed = 1024 + 7168 * settings.speed / 100; // Q12 per second
        for (Comet &c: _comets) {
            if (!c.on) continue;
            const int32_t from = c.pos;
            c.pos = int16_t(c.pos + speed * (4 + c.hue / 85) / 4 * _dt / 1000);
            if (!m) {
                const int a = (from * (n - 1)) >> 12, b = mini(n - 1, (c.pos * (n - 1)) >> 12);
                for (int k = a; k <= b; k++) stamp(k, u8q(c.energy), c.hue);
            } else {
                const int32_t r0 = (from * rmax) >> 12, r1 = (c.pos * rmax) >> 12;
                const int32_t e = (c.energy * (ONE - ((2458 * c.pos) >> 12))) >> 12;
                const int32_t lo = maxi(0, r0 - width), hi = r1 + width;
                const uint32_t lo2 = uint32_t(lo * lo), hi2 = uint32_t(hi * hi);
                for (int y = 0; y < H; y++)
                    for (int x = 0; x < W; x++) {
                        const int32_t dx = x * 16 - cx, dy = y * 16 - cy;
                        const uint32_t d2 = uint32_t(dx * dx) + uint32_t(dy * dy);
                        if (d2 < lo2 || d2 > hi2) continue;
                        const int32_t r = int32_t(isqrt(d2)), d = r < r0 ? r0 - r : (r > r1 ? r - r1 : 0);
                        if (d < width) stamp(y * W + x, u8q(e * (width - d) / width), c.hue);
                    }
            }
            if (c.pos >= ONE) c.on = false;
        }
        const uint8_t glow = uint8_t((P(2) * line) >> 12);
        const int32_t tone = u8q(_tone);
        if (!m) {
            for (int x = 0; x < W; x++) {
                const int c = stripCell(x);
                if (c < 0) { stripPut(x, 0, 0); continue; }
                stripPut(x, _v[c] > glow ? _v[c] : glow, _v[c] > glow ? _h[c] : tone);
            }
            return;
        }
        for (int i = 0, y = 0; y < H; y++)
            for (int x = 0; x < W; x++, i++) put(x, y, _v[i] > glow ? _v[i] : glow, _v[i] > glow ? _h[i] : tone);
    }
    // Sparkles: treble and onsets sprinkle sparks, bass is a soft glow.
    // Parameters: treble density (64 = x1), sparks per hit, bass glow, colours (0 by group, 1 random, 2 by tone).
    void sparkles() {
        using namespace detail;
        const int n = W * H, colors = P(3) % 3;
        fade(n, releaseMs() * 6 / 10);
        int32_t strength[3] = {0, 0, 0};
        const int mask = onsets(strength);
        // A faint idle twinkle plus the treble's sparks, x density / 64.
        const uint32_t rate = uint32_t(5 + ((307 * level01(_host->level(2))) >> 12) * P(0) / 64);
        _spawn += int32_t(uint32_t(_dt) * uint32_t(n) * rate / 1000);
        for (int g = 0; g < 3; g++) if (mask & (1 << g)) _spawn += (((n * P(1)) >> 4) * strength[g]) >> 8;
        const int32_t tone = (_tone * 255) >> 12;
        for (int k = 0; k < n && _spawn >= 256; k++, _spawn -= 256) {
            const int g = mask & 4 ? 2 : (mask & 2 ? 1 : (mask & 1 ? 0 : 2));
            stamp(random(n), uint8_t(191 + random8() / 4), colors == 1 ? random8() : colors == 2 ? tone + random8() / 8 : g * 85 + random8() / 8);
        }
        _spawn = mini(_spawn, n << 8);
        const uint8_t glow = uint8_t((P(2) * level01(_host->level(0))) >> 12);
        const int32_t glowHue = (_tone * 120) >> 12;
        for (int i = 0, y = 0; y < H; y++)
            for (int x = 0; x < W; x++, i++) put(x, y, _v[i] > glow ? _v[i] : glow, _v[i] > glow ? _h[i] : glowHue);
    }
    // Mood: the whole lamp breathes with the level, colour follows the tone.
    // Parameters: colour spread across the lamp, bass flash, tone range, map
    // (0 one level for the lamp, 1 spectrum: each place breathes with its band, bass to treble).
    void mood() {
        using namespace detail;
        const bool m = matrix(), spectral = P(3) % 2;
        _mood = int16_t(approach(_mood, level01(_host->control()), 50, releaseMs()));
        const int32_t flash = (P(1) * 16 * _host->pulse(0)) >> 12;
        const uint8_t v = u8q(_mood + flash);
        const int32_t hue = (_tone * P(2)) >> 12, cells = stripCells();
        int32_t lv[MAX_BANDS];
        const int n = spectral ? bands() : 0;
        if (spectral)
            for (int i = 0; i < n; i++) {
                _level[i] = int16_t(approach(_level[i], level01(autolevel(i, _host->band(i), _dt)), 50, releaseMs()));
                lv[i] = _level[i];
            }
        for (int y = 0; y < H; y++)
            for (int x = 0; x < W; x++) {
                const int sc = m ? 0 : stripCell(x);
                // Position 0..ONE: the diagonal of a matrix, the cells of a strip.
                const int32_t t = m ? (x + y) * ONE / maxi(1, W + H - 2) : (sc < 0 ? 0 : sc * ONE / maxi(1, cells - 1));
                const uint8_t here = spectral ? u8q(spectrumAt(lv, n, t) + flash) : v;
                put(x, y, here, hue + ((t * P(0)) >> 12));
            }
    }
    // Waveform: strip = coloured loudness history, matrix = history as bars / mirrored waveform.
    // Parameter: colours (0 classic: by level on a strip, by height on a matrix; 1 along the history; 2 by tone).
    void waveform() {
        using namespace detail;
        const int colors = P(0) % 3;
        const int32_t tone = (_tone * 255) >> 12;
        if (!matrix()) {
            const int cells = stripCells();
            for (int x = 0; x < W; x++) {
                const int c = stripCell(x);
                const uint8_t v = c < 0 ? 0 : u8q(level01(_host->wave(c, cells)));
                stripPut(x, v, colors == 1 ? c * 255 / maxi(1, cells - 1) : colors == 2 ? tone : v);
            }
            return;
        }
        const int rows = riseCells(settings.mirror);
        for (int x = 0; x < W; x++) {
            const int xp = _host->originX(x);
            const int32_t a = xp < 0 ? 0 : level01(_host->wave(xp, W)) * rows;
            for (int y = 0; y < H; y++) {
                const int r = riseCell(y, settings.mirror);
                const int32_t hue = colors == 1 ? xp * 255 / maxi(1, W - 1) : colors == 2 ? tone : r * 255 / maxi(1, rows - 1);
                put(x, y, xp < 0 ? 0 : u8((a - (int32_t(r) << 12)) >> 4), hue);
            }
        }
    }
    // Bass / Mid / Treble meters: three segments of a strip, three bars on a matrix.
    void bandMeters() {
        using namespace detail;
        for (int g = 0; g < 3; g++) meter(g, level01(autolevel(g, _host->level(g), _dt)));
        for (int x = 0; x < W; x++) {
            const int xp = _host->originX(x), g = xp < 0 ? 0 : mini(2, xp * 3 / W), start = g * W / 3, end = (g + 1) * W / 3;
            const int len = maxi(1, end - start);
            const bool gap = xp < 0 || (W >= 12 && g > 0 && xp == start);
            if (!matrix()) {
                const int cells = settings.mirror ? (len + 1) / 2 : len, local = xp - start, mid = (len - 1) / 2;
                const int c = settings.mirror ? (local <= mid ? mid - local : local - mid - ((len & 1) ? 0 : 1)) : local;
                stripPut(x, gap ? 0 : meterV(c, cells, _level[g], _peak[g]), g * 85 + 20);
                continue;
            }
            const int rows = riseCells(settings.mirror);
            for (int y = 0; y < H; y++)
                put(x, y, gap ? 0 : meterV(riseCell(y, settings.mirror), rows, _level[g], _peak[g]), g * 85 + 20);
        }
    }
    // Juggle: eight dots, one per band range, swing with their band and leave trails.
    // Parameters: dots (1..8 band ranges), dot size (128 = x1).
    void juggle() {
        using namespace detail;
        const bool m = matrix();
        const int n = m ? W * H : stripCells(), dots = clampi(P(0), 1, 8);
        fade(n, releaseMs() * 45 / 100);
        const int32_t level = overall(_host->lineSource());
        const int32_t speed = 410 + 1638 * settings.speed / 100, pace = 614 + ((819 * level) >> 12); // Q10
        for (int k = 0; k < dots; k++) {
            const int32_t b = level01(autolevel(k, bandCell(k, dots), _dt));
            const int32_t rate = (((11796 + 2949 * k) * speed >> 10) * pace) >> 10; // (.18 + .045k) turns/s
            _phase[k] = advance(_phase[k], rate);
            _phase2[k] = advance(_phase2[k], rate * 1403 >> 10);                       // x 1.37
            if (b < 205) continue;
            const int hue = k * 255 / dots;
            if (!m) {
                // Swing around the strip (Q8 positions); wider swings for louder bands.
                const int32_t pos = (wave16(_phase[k]) * (n - 1)) >> 8, half = 154 + (((b >> 3) * P(1)) >> 7);
                for (int i = maxi(0, (pos - half) >> 8); i <= mini(n - 1, ((pos + half) >> 8) + 1); i++)
                    stamp(i, u8(((b >> 4) * clampi(half - absi((i << 8) - pos) + 128, 0, 256)) >> 8), hue);
            } else {
                // Lissajous paths (Q4 positions), the dot grows with its band.
                const int32_t px = (wave16(_phase[k]) * (W - 1)) >> 12;
                const int32_t py = (wave16(uint16_t(_phase2[k] + k * 7209)) * (H - 1)) >> 12;
                const int32_t rad = 11 + ((b * maxi(16, mini(W, H) * 16 * P(1) / 1280)) >> 12);
                for (int y = maxi(0, (py - rad - 16) >> 4); y <= mini(H - 1, (py + rad + 16) >> 4); y++)
                    for (int x = maxi(0, (px - rad - 16) >> 4); x <= mini(W - 1, (px + rad + 16) >> 4); x++)
                        stamp(y * W + x, u8(((b >> 4) * clampi(rad - dist(x * 16 - px, y * 16 - py) + 8, 0, 16)) >> 4), hue);
            }
        }
        if (!m) {
            for (int x = 0; x < W; x++) { const int c = stripCell(x); stripPut(x, c < 0 ? 0 : _v[c], c < 0 ? 0 : _h[c]); }
            return;
        }
        for (int i = 0, y = 0; y < H; y++) for (int x = 0; x < W; x++, i++) put(x, y, _v[i], _h[i]);
    }

    // ---- constructors -------------------------------------------------------
    // The canvas _v / _h of `cols` x `rows` cells (a strip: one column along
    // it) drawn onto the LEDs: matrix rows from the bottom (or the middle row
    // when mirrored), strip cells from the origin (or both ways from the centre).
    void showCanvas(bool m, int cols) {
        using namespace detail;
        if (!m) {
            for (int x = 0; x < W; x++) { const int c = stripCell(x); stripPut(x, c < 0 ? 0 : _v[c], c < 0 ? 0 : _h[c]); }
            return;
        }
        for (int x = 0; x < W; x++) {
            const int xp = _host->originX(x);
            for (int y = 0; y < H; y++) {
                if (xp < 0) { put(x, y, 0, 0); continue; }
                const int i = riseCell(y, settings.mirror) * cols + xp;
                put(x, y, _v[i], _h[i]);
            }
        }
    }
    // A dot at (px, py) (Q8 cells) spread over its four cells. The weights are
    // doubled: a dot between two cells lights both fully instead of both at half.
    void dot(int32_t px, int32_t py, int cols, int rows, int32_t v, int32_t hue) {
        using namespace detail;
        if (px < 0 || py < 0) return;
        const int x0 = px >> 8, y0 = py >> 8;
        const int32_t fx = px & 255, fy = py & 255;
        for (int dy = 0; dy < 2; dy++)
            for (int dx = 0; dx < 2; dx++) {
                const int x = x0 + dx, y = y0 + dy;
                if (x >= cols || y >= rows) continue;
                const int32_t w = mini(256, ((dx ? fx : 256 - fx) * (dy ? fy : 256 - fy)) >> 7);
                stamp(y * cols + x, u8q((v * w) >> 8), hue);
            }
    }

    // Particles: hits launch sparks that fly, fall and fade, leaving trails.
    // Parameters: shape (0 fountain from the bottom / the start, 1 rain from
    // the top / the end, 2 bursts at random points, 3 bubbles rising with the
    // gravity turned over, 4 comets across), trigger (0 bands: each band's hit
    // launches from its own place, bass on the left, treble on the right / further;
    // 1..3 bass / mid / treble hits, 4 any hit, 5 beats, 6 a steady stream with
    // the level), amount per hit, speed, gravity (128 none, above pulls down),
    // spread, life, trail, colours (0 by frequency, 1 by tone, 2 random, 3
    // cooling with age), brightness. Harder hits throw sparks faster and they live longer.
    void particles() {
        using namespace detail;
        const bool m = matrix();
        const int rows = m ? riseCells(settings.mirror) : stripCells(), cols = m ? W : 1;
        const int shape = P(0) % 5, trigger = P(1) % 7, amount = 1 + P(2) * 15 / 255, colors = P(8) % 4;
        // Launch speed .25 .. 5.3 lamps per second, gravity up to 6 lamps / s^2 (Q12).
        const int32_t speed = 1024 + P(3) * 81, gravity = (P(4) - 128) * 193 * (shape == 3 ? -1 : 1), life = 150 + P(6) * 15;
        fade(rows * cols, 20 + P(7) * 4);
        int32_t strength[4] = {0, 0, 0, 0};
        const int mask = hits(strength);
        const int32_t tone = (_tone * 255) >> 12;
        // One spark with energy .5..1 (Q12) from place t (Q16: across a matrix,
        // along a strip; -1 anywhere); bursts start at (cx, cy).
        auto launch = [&](int32_t energy, int32_t hue, int32_t t, int32_t cx, int32_t cy) {
            Spark *slot = &_sparks[0];
            for (Spark &s: _sparks) { if (!s.on) { slot = &s; break; } if (s.life < slot->life) slot = &s; }
            Spark &s = *slot;
            const int32_t v = (((speed * energy) >> 12) * (192 + (random8() >> 2))) >> 8; // 75..100 % of it
            const int32_t side = ((int32_t(random8()) - 128) * P(5)) >> 8;                // -128..127 x spread
            const int32_t any = (int32_t(random8()) << 8) | random8();
            s.on = 1;
            s.life = int16_t((energy * (192 + (random8() >> 2))) >> 8);
            s.hue = uint8_t(clampi(hue, 0, 255));
            s.x = m ? (t >= 0 ? t : any) : 32768;
            switch (shape) {
                case 0: // fountain: up, fanned out by the spread; on a strip treble flies further
                    s.y = 0; s.vy = int16_t(!m && t >= 0 ? (v * (2048 + (t >> 4))) >> 12 : v); s.vx = int16_t(m ? (v * side) >> 8 : 0);
                    break;
                case 1: // rain: falls from the top
                    s.y = 65535; s.vy = int16_t(-(!m && t >= 0 ? (v * (1024 + (t >> 4))) >> 13 : v >> 2)); s.vx = int16_t(m ? (v * side) >> 9 : 0);
                    break;
                case 2: { // burst: every direction from (cx, cy)
                    const uint16_t a = uint16_t(any);
                    const int32_t sv = (v * (256 - (P(5) >> 1) + ((random8() * (P(5) >> 1)) >> 8))) >> 8;
                    s.x = m ? cx : 32768; s.y = cy;
                    s.vx = int16_t(m ? (sv * _host->sin16(uint16_t(a + 16384))) >> 15 : 0);
                    s.vy = int16_t(m ? (sv * _host->sin16(a)) >> 15 : (a & 1 ? sv : -sv) >> 1);
                    break;
                }
                case 3: // bubbles: rise slowly and wobble
                    s.y = m || t < 0 ? 0 : t >> 1; s.vy = int16_t(v >> 2); s.vx = 0;
                    break;
                default: // comets: across a matrix at the height of their band, along a strip
                    if (m) { s.x = 0; s.y = t >= 0 ? t : any; s.vx = int16_t(v); s.vy = int16_t((v * side) >> 10); }
                    else { s.y = 0; s.vy = int16_t(v); s.vx = 0; }
            }
        };
        // A burst centre: x 20..80 %, y 40..85 % of a matrix, 20..80 % of a strip.
        int32_t cx = 32768, cy = 32768;
        auto centre = [&](int32_t t) {
            cx = t >= 0 ? t : 13107 + random(39321);
            cy = m ? 26214 + random(29491) : t >= 0 ? t : 13107 + random(39321);
        };
        auto hueOf = [&](int32_t own) { return colors == 1 ? tone : colors == 2 ? int32_t(random8()) : own; };
        if (trigger == 0) {
            // A broadband hit fires many bands at once: they share the amount, at most 2x per frame.
            uint8_t hit_band[MAX_BANDS];
            int16_t hit_e[MAX_BANDS];
            int count = 0;
            bandHits([&](int i, int, int32_t e) { hit_band[count] = uint8_t(i); hit_e[count++] = int16_t(e); });
            const int n = bands(), budget = amount * 2, stride = (count + budget - 1) / budget;
            const int each = clampi(budget / maxi(1, count), 1, amount);
            for (int k = 0; k < count; k += maxi(1, stride)) {
                const int i = hit_band[k];
                const int32_t t = n > 1 ? i * 65535 / (n - 1) : 32768;
                if (shape == 2) centre(t);
                for (int j = 0; j < each; j++) launch(2048 + (hit_e[k] >> 1), hueOf(bandHue(i, n)), t, cx, cy);
            }
        } else {
            int group = 3, emit = 0;
            int32_t energy = 0;
            if (trigger == 6) {
                const int32_t level = overall(_host->lineSource());
                _spawn += _dt * amount * level / 4000; // up to 4 x amount sparks per second (Q8)
                emit = mini(amount * 2, _spawn >> 8);
                _spawn = mini(_spawn - (emit << 8), 255);
                energy = 2048 + (level >> 1);
            } else {
                // Triggers 1..5 are fired()'s 0..4.
                energy = fired(trigger - 1, mask, strength, group);
                if (energy) { emit = amount; energy = 2048 + (energy >> 1); }
            }
            // Fountains and comets start at the place of their group (bass left / low), the rest anywhere.
            const int32_t place = group < 3 && (shape == 0 || shape == 4) ? 10923 + group * 21845 : shape == 0 ? 32768 : -1;
            if (emit && shape == 2) centre(-1);
            for (int j = 0; j < emit; j++) launch(energy, hueOf(trigger == 6 ? tone : groupHue(group)), m || shape == 2 ? place : -1, cx, cy);
        }
        // Move: gravity pulls along -y; bubbles wobble, bursts slow down.
        for (Spark &s: _sparks) {
            if (!s.on) continue;
            int32_t vx = s.vx, vy = s.vy - gravity * _dt / 1000;
            if (shape == 3) { vx += (((int32_t(random8()) - 128) * P(5)) >> 5) * _dt / 100; vx -= vx * _dt / 400; }
            if (shape == 2) { vx -= vx * _dt / 1200; vy -= vy * _dt / 1200; }
            s.vx = int16_t(clampi(vx, -30000, 30000)); s.vy = int16_t(clampi(vy, -30000, 30000));
            s.x += s.vx * _dt * 16 / 1000;
            s.y += s.vy * _dt * 16 / 1000;
            s.life = int16_t(s.life - ONE * _dt / life);
            if (s.life <= 0 || s.y < -4096 || s.y > 69632 || (m && (s.x < 0 || s.x > 65535))) { s.on = 0; continue; }
            const int32_t hue = colors == 3 ? s.hue + (((ONE - s.life) * 120) >> 12) : s.hue;
            // Full light for most of the life (a weak hit starts at 2/3), fading over its last part.
            dot(m ? (s.x * (cols - 1)) >> 8 : 0, (s.y * (rows - 1)) >> 8, cols, rows, mini(ONE, s.life * 7 / 4), hue);
        }
        showCanvas(m, cols);
    }

    // Zones: the lamp split into zones that flash on hits or follow their band.
    // Parameters: zones (1..16), layout (0 columns, 1 rows, 2 rings, 3
    // sectors, 4 tiles; a strip is always segments), trigger (0 bands: each
    // zone is an equalizer band, 1 band hits: the zone of the band that hit,
    // 2..4 bass / mid / treble hits, 5 any hit, 6 beats), pattern for hits (0
    // all, 1 chase, 2 random, 3 alternate, 4 bounce, 5 build up), decay, glow
    // with the level, colours (0 by zone, 1 by what hit: its band or group, 2 a
    // new colour each hit, 3 by tone), edge (0 flat zones .. 255 dark borders), brightness.
    void zones() {
        using namespace detail;
        const bool m = matrix();
        const int Z = clampi(P(0), 1, ZONES_MAX), layout = P(1) % 5, trigger = P(2) % 7, pattern = P(3) % 6, colors = P(6) % 4;
        const int32_t decay = 50 + P(4) * 6, edge = P(7);
        int32_t strength[4] = {0, 0, 0, 0};
        const int mask = hits(strength);
        const int32_t line = overall(_host->lineSource()), glow = (P(5) * line) >> 9;
        auto flash = [&](int z, int32_t v, int32_t hue) {
            if (_zones[z].level < v) _zones[z].level = int16_t(v);
            _zones[z].hue = uint8_t(clampi(hue, 0, 255));
        };
        if (trigger == 0) {
            // An equalizer: each zone follows its part of the spectrum; "a new colour" rotates slowly.
            _clock += _dt;
            for (int z = 0; z < Z; z++) {
                const int32_t target = level01(autolevel(z, bandCell(z, Z), _dt));
                _zones[z].level = int16_t(approach(_zones[z].level, target, 20, decay));
                _zones[z].hue = uint8_t(colors == 2 ? (_clock / 40 + z * 256 / Z) & 255 : bandHue(z, Z));
            }
        } else {
            for (int z = 0; z < ZONES_MAX; z++) _zones[z].level = int16_t(approach(_zones[z].level, 0, 1, decay));
            if (trigger == 1) {
                bandHits([&](int i, int n, int32_t e) {
                    _hit_hue = uint8_t(_hit_hue + 47);
                    flash(i * Z / n, mini(ONE, 1229 + e * 3 / 4), colors == 2 ? _hit_hue : bandHue(i, n));
                });
            } else {
                int g = 0;
                const int32_t e = fired(trigger - 2, mask, strength, g);
                if (e) {
                    const int32_t v = mini(ONE, 1229 + e * 3 / 4);
                    _hit_hue = uint8_t(_hit_hue + 47);
                    const int32_t hue = colors == 2 ? _hit_hue : groupHue(g);
                    // The first hit lands on zone 0.
                    if (_cursor >= Z) { _cursor = uint8_t(pattern == 4 ? mini(1, Z - 1) : Z - 1); _dir = -1; }
                    switch (pattern) {
                        case 0: for (int z = 0; z < Z; z++) flash(z, v, hue); break;
                        case 1: _cursor = uint8_t(_cursor + 1 >= Z ? 0 : _cursor + 1); flash(_cursor, v, hue); break;
                        case 2: { int z = random(Z); if (Z > 1 && z == _cursor) z = (z + 1) % Z; _cursor = uint8_t(z); flash(z, v, hue); break; }
                        case 3: _count = _count ? 0 : 1; for (int z = _count; z < Z; z += 2) flash(z, v, hue); break;
                        case 4:
                            if (Z > 1) { if (_cursor + _dir < 0 || _cursor + _dir >= Z) _dir = int8_t(-_dir); _cursor = uint8_t(_cursor + _dir); }
                            flash(_cursor, v, hue); break;
                        default: _count = uint8_t(_count >= Z ? 1 : _count + 1); for (int z = 0; z < _count; z++) flash(z, v, hue);
                    }
                }
            }
        }
        const int cells = m ? 0 : stripCells();
        const int32_t cx = (W - 1) * 8, cy = radial() ? row16() : (H - 1) * 8;
        const int32_t maxd = maxi(16, dist(maxi(cx, W * 16 - 16 - cx), maxi(cy, H * 16 - 16 - cy)));
        const int rows = riseCells(settings.mirror), tiles = int(isqrt(uint32_t(Z - 1))) + 1;
        const int32_t tone = (_tone * 255) >> 12;
        // Zone of position pos in [0, span) and the distance from the zone's middle, 0..255.
        auto zoneOf = [&](int32_t pos, int32_t span, int parts, int32_t &e) {
            const int32_t q = pos * parts * 256 / maxi(1, span), z = mini(parts - 1, q >> 8);
            e = absi(2 * (q - (z << 8)) - 255);
            return int(z);
        };
        for (int x = 0; x < W; x++) {
            const int xp = m ? _host->originX(x) : stripCell(x);
            for (int y = 0; y < H; y++) {
                if (xp < 0) { put(x, y, 0, 0); continue; }
                int z = 0;
                int32_t e = 0;
                if (!m) z = zoneOf(xp * 16 + 8, cells * 16, Z, e);
                else if (layout == 0) z = zoneOf(xp * 16 + 8, W * 16, Z, e);
                else if (layout == 1) z = zoneOf(riseCell(y, settings.mirror) * 16 + 8, rows * 16, Z, e);
                else if (layout == 2) z = zoneOf(dist(xp * 16 - cx, y * 16 - cy), maxd + 1, Z, e);
                else if (layout == 3) {
                    const int32_t q = int32_t(angle16(y * 16 - cy, xp * 16 - cx)) * Z >> 8;
                    z = mini(Z - 1, q >> 8); e = absi(2 * (q & 255) - 255);
                } else {
                    int32_t ex, ey;
                    const int gx = zoneOf(xp * 16 + 8, W * 16, tiles, ex), gy = zoneOf(y * 16 + 8, H * 16, tiles, ey);
                    z = (gy * tiles + gx) % Z; e = maxi(ex, ey);
                }
                const int32_t v = (_zones[z].level * (ONE - (((e * edge) >> 8) << 4))) >> 12;
                const int32_t hue = colors == 0 ? bandHue(z, Z) : colors == 3 ? tone + z * 12 : _zones[z].hue;
                put(x, y, u8q(maxi(v, glow)), hue);
            }
        }
    }

    // Melody: follows the dominant frequency (the loudest band, refined between
    // bands) inside a part of the spectrum. Parameters: from, to (the part of
    // the spectrum, 0..255 of the bands), style (0 a dot: along a strip; on a
    // matrix across, as high as its level; 1 a bar: from the origin up to the
    // note on a strip, the note's column on a matrix; 2 a trace: the note over
    // time scrolling across a matrix, on a strip a dot), glide, size, trail
    // (the trace's scroll speed), colours (0 by the note, 1 by tone, 2 by
    // level), gate (quieter notes are ignored), brightness.
    void melody() {
        using namespace detail;
        const bool m = matrix();
        const int n = bands(), style = P(2) % 3, colors = P(6) % 3;
        int lo = P(0) * (n - 1) / 255, hi = P(1) * (n - 1) / 255;
        if (hi < lo) { const int t = lo; lo = hi; hi = t; }
        if (hi == lo) { if (hi < n - 1) hi++; else lo--; }
        int best = lo;
        for (int i = lo; i <= hi; i++) if (_host->band(i) > _host->band(best)) best = i;
        const int32_t peak = _host->band(best);
        int32_t note = -1; // position inside the part, Q12
        if (peak > P(7) * 16) {
            // A parabola through the peak and its neighbours puts the note between bands (Q8 of a band).
            int32_t shift = 0;
            if (best > lo && best < hi) {
                const int32_t l = _host->band(best - 1), r = _host->band(best + 1), den = l - 2 * peak + r;
                if (den < 0) shift = clampi(((l - r) << 7) / den, -128, 128);
            }
            note = clampi(((best - lo) * 256 + shift) * ONE / ((hi - lo) * 256), 0, ONE);
        }
        // _grow[0]: the gliding note, _grow[1]: its level. A note after a pause jumps.
        const int32_t level = note < 0 ? 0 : level01(peak), glide = 10 + P(3) * 3;
        if (note >= 0) _grow[0] = int16_t(_grow[1] > 82 ? approach(_grow[0], note, glide, glide) : note);
        _grow[1] = int16_t(approach(_grow[1], level, 20, 60 + P(5) * 2));
        // The light follows the square root of the level: a quiet note still shows.
        const int32_t pos = _grow[0], v = int32_t(isqrt(uint32_t(_grow[1]) << 12));
        const int32_t hue = colors == 0 ? 20 + ((pos * 170) >> 12) : colors == 1 ? (_tone * 255) >> 12 : (v * 255) >> 12;
        const int rows = m ? riseCells(settings.mirror) : stripCells(), cols = m ? W : 1;
        const int32_t size = 128 + P(4) * 4; // half width, 1/256 cell: .5 .. 4.5 cells
        // Cover of a cell `d` (1/256 cell) from the middle: full inside, a one-cell soft edge.
        auto cover = [&](int32_t d) { return clampi(size + 128 - d, 0, 256); };
        if (style == 2 && m) {
            // Trace: the canvas scrolls left one column per step, the newest note on the right.
            _clock += _dt;
            const int32_t step = 20 + P(5) / 2;
            for (int k = 0; _clock >= step && k < 8; k++) {
                _clock -= step;
                for (int r = 0; r < rows; r++) {
                    uint8_t *row = _v + r * cols, *hues = _h + r * cols;
                    for (int c = 0; c < cols - 1; c++) { row[c] = row[c + 1]; hues[c] = hues[c + 1]; }
                    row[cols - 1] = 0;
                }
            }
            const int32_t at = (pos * (rows - 1)) >> 4; // Q8 row
            for (int r = 0; r < rows; r++) {
                const int32_t w = cover(absi((r << 8) - at));
                if (w) stamp(r * cols + cols - 1, u8q((v * w) >> 8), hue);
            }
            showCanvas(m, cols);
            return;
        }
        fade(rows * cols, 20 + P(5) * 4);
        if (!m) {
            const int32_t along = (pos * (rows - 1)) >> 4;
            for (int c = 0; c < rows; c++) {
                const int32_t w = style == 1 ? clampi(along + 128 - (c << 8), 0, 256) : cover(absi((c << 8) - along));
                if (w) stamp(c, u8q((v * w) >> 8), hue);
            }
        } else {
            // A dot as high as the level, or the note's column filled up to it.
            const int32_t along = (pos * (cols - 1)) >> 4, up = (v * (rows - 1)) >> 4, height = v * rows;
            for (int c = 0; c < cols; c++) {
                const int32_t w = cover(absi((c << 8) - along));
                if (!w) continue;
                for (int r = 0; r < rows; r++) {
                    const int32_t h = style == 1 ? clampi((height - (int32_t(r) << 12)) >> 4, 0, 256) : (cover(absi((r << 8) - up)) * v) >> 12;
                    if (h) stamp(r * cols + c, u8((h * w) >> 8), hue);
                }
            }
        }
        showCanvas(m, cols);
    }

    // Groove: a pattern that moves with the beat, one step per beat that eases
    // out from the beat itself. A confident tempo runs it from the beat phase;
    // without one, bass and mid hits start the steps. Bass hits (and the beat)
    // kick it outwards, a mid hit turns the next step round, the treble adds small sparks.
    // Parameters: shape (0 rings from the centre, 1 spin: spiral arms, 2
    // stripes, 3 bounce: a ball swinging side to side; on a strip waves run
    // along it, or a bar swings), step (128 = one period of the pattern per
    // beat), snap (0 a steady glide .. 255 a sharp step on the beat), pump (the
    // expansion), zoom, sparks, colours (0 along the pattern, 1 by tone, 2 a
    // new colour each beat), brightness.
    void groove() {
        using namespace detail;
        const bool m = matrix();
        Groove &g = _groove;
        const int shape = P(0) % 4, colors = P(6) % 3;
        int32_t strength[3] = {0, 0, 0};
        const int mask = onsets(strength);
        // The beat clock. Locked to a confident tempo, a phase-locked loop pulls
        // it towards the host's phase and its rate towards the tempo.
        const int32_t conf = _host->tempoConfidence();
        g.locked = conf > 1229 ? 1 : conf < 819 ? 0 : g.locked;
        uint32_t from = g.pos;
        bool beat = false;
        if (g.locked) {
            const int32_t err = int16_t(uint16_t(_host->beatPhase() - uint16_t(g.pos)));
            g.rate = clampi(g.rate + err * 9 * _dt / 1000, 26214, 327680); // 24 .. 300 BPM
            g.pos += uint32_t(maxi(0, (g.rate + err * 6) * _dt / 1000));  // never backwards
        } else {
            // A hit starts the next step from where the pattern is; each step
            // runs at the last tempo (at least 120 BPM) and stops at its end.
            if (mask & 3) { from = g.pos = (g.pos | 0xffff) + 1; beat = true; }
            const uint32_t left = 0xffff - (g.pos & 0xffff);
            g.pos += mini(left, maxi(g.rate, 131072) * _dt / 1000);
        }
        const bool crossed = (g.pos >> 16) != (from >> 16);
        if (beat || crossed) g.hue = uint8_t(g.hue + (colors == 2 ? 72 : 6));
        // The step done at a phase (Q16): t glides, 1 - (1 - t)^4 snaps.
        auto ease = [&](uint32_t pos) {
            const int32_t t = int32_t(pos & 0xffff), r = (65536 - t) >> 8, r2 = (r * r) >> 8;
            return t + ((65536 - r2 * r2 - t) * P(2) >> 8);
        };
        // The pattern travels `step` / 128 periods per beat; a mid hit turns the next step round.
        if (mask & 2) g.turn = 1;
        int32_t done = ease(g.pos) - ease(from);
        if (crossed || beat) {
            if (crossed) g.travel += uint32_t(g.dir * (((65536 - ease(from)) * P(1)) >> 7));
            if (g.turn) { g.dir = int8_t(-g.dir); g.turn = 0; }
            done = ease(g.pos);
        }
        g.travel += uint32_t(g.dir * ((done * P(1)) >> 7));
        // Expansion (Q12): a kick from a bass hit, or from the beat itself while locked.
        const int32_t rest = (65536 - int32_t(g.pos & 0xffff)) >> 4, rest2 = (rest * rest) >> 12;
        const int32_t env = g.locked ? ((rest2 * rest2) >> 12) * 3 / 4 : 0;
        g.boost = int16_t((mask & 1) && strength[0] > g.boost ? strength[0] : approach(g.boost, 0, 1, 150));
        const int32_t e = maxi(g.boost, env), grow = (e * P(3)) >> 8;
        _mood = int16_t(approach(_mood, overall(_host->lineSource()), 30, releaseMs()));
        const int32_t amp = mini(ONE, _mood * 4);
        // Sparks: a few with the treble, more on a treble hit; gone in a fraction of a beat.
        const int n = m ? W * H : stripCells();
        fade(n, 150);
        _spawn += ((n + 15) >> 4) * ((P(5) * level01(_host->level(2))) >> 11) * _dt / 1000; // Q8, n / 16 per second x sparks / 128
        if (mask & 4) _spawn += ((n + 15) >> 4) * P(5) * (strength[2] >> 4) / 128;
        for (_spawn = mini(_spawn, n << 8); _spawn >= 256; _spawn -= 256) stamp(random(n), 255, (g.hue + 128) & 255);
        // The pattern: phase per 1/16 LED (Q16 periods), (8 + zoom) / 32 periods across, the expansion stretches it up to 2x.
        const int cells = m ? W : stripCells();
        const int32_t size = m ? maxi(W, H) : cells, k0 = 128 * (8 + P(4)) / maxi(1, size);
        const int32_t k = k0 - ((k0 * grow) >> 13);
        const int arms = 1 + ((P(4) + 32) >> 6);
        const int32_t cx = m ? (W - 1) * 8 : 0, cy = radial() ? row16() : (H - 1) * 8;
        // Bounce: the ball rests at the sides and swings through the middle, lower there (Q4).
        const int32_t swing = _host->sin16(uint16_t(g.travel + 16384)), dip = _host->sin16(uint16_t(2 * g.travel + 16384));
        const int32_t span = (m ? W : cells) * 16, bx = m ? cx + ((swing * span * 3 / 10) >> 15) : (cells * 8) + ((swing * span * 3 / 8) >> 15);
        const int32_t by = cy + ((dip * H * 4) >> 15), radius = (size * 16 * (900 + ((800 * grow) >> 12))) >> 12;
        const int32_t tone = (_tone * 255) >> 12;
        auto light = [&](int i, int32_t dx, int32_t dy) -> uint32_t { // brightness << 8 | hue
            uint32_t s; // the place in the pattern for the colours: Q16 periods, no seams
            int32_t p;
            if (shape == 3) {
                // The ball with a soft edge and a glow around it.
                const int32_t d = dist(dx - (m ? bx - cx : bx), m ? dy - (by - cy) : 0);
                p = maxi(clampi((radius + 16 - d) * ONE / 32, 0, ONE), clampi((2 * radius - d) * 1024 / maxi(1, radius), 0, 1024));
                s = uint32_t(d * 2048);
            } else {
                uint32_t a = 0; // spin: the arms, whole periods round the centre
                if (!m || shape == 0) s = uint32_t(dist(dx, dy) * k);
                else if (shape == 1) { s = uint32_t(dist(dx, dy) * k * 3 / 4); a = angle16(dy, dx) * uint32_t(arms); }
                else s = uint32_t((dx + dy) * k * 3 / 8);
                s -= g.travel;
                // Bright crests, and a glow of a quarter between them.
                p = clampi(1024 + ((wave16(uint16_t(s + a)) >> 4) - 1024) * 3 / 2, 1024, ONE);
            }
            int32_t v = (p * amp) >> 12;
            int32_t hue = colors == 0 ? g.hue + (s >> 11) : colors == 1 ? mini(255, tone + ((s >> 12) & 15)) : g.hue + ((s >> 12) & 15);
            if (_v[i] > u8q(v)) { v = _v[i] << 4; hue = _h[i]; }
            return uint32_t(u8q(v)) << 8 | uint8_t(hue);
        };
        if (!m) {
            for (int x = 0; x < W; x++) {
                const int c = stripCell(x);
                const uint32_t l = c < 0 ? 0 : light(c, c * 16, 0);
                stripPut(x, uint8_t(l >> 8), l & 255);
            }
            return;
        }
        for (int x = 0; x < W; x++) {
            const int xp = _host->originX(x);
            for (int y = 0; y < H; y++) {
                if (xp < 0) { put(x, y, 0, 0); continue; }
                const int32_t dy = y * 16 - cy;
                const uint32_t l = light(y * W + xp, xp * 16 - cx, settings.mirror ? absi(dy) : dy);
                put(x, y, uint8_t(l >> 8), l & 255);
            }
        }
    }

#if VIZ_WITH_HEAVY
    // Fire: flames rise from the bottom (matrix) or from the start / centre
    // (strip); the spectrum, the bass or the loudness feed them, hits throw up flares.
    // Parameters: fuel (0 bands: a flame per band, 1 bass, 2 loudness, 3 hits
    // only), height, intensity, flares, wind (128 still), blend (neighbouring
    // flames merge), colours (0 heat: black-red-yellow-white, 1 palette by heat,
    // 2 palette colour by tone), embers (a bed glowing with the loudness).
    void fire() {
        using namespace detail;
        const bool m = matrix();
        const int32_t step = 33 - 22 * settings.speed / 100, level = overall(_host->lineSource());
        const int fuel = P(0) % 4, n = bands();
        int32_t strength[3] = {0, 0, 0};
        const int mask = onsets(strength);
        const int32_t flare = (mask & 1 ? strength[0] : 0) + (mask & 2 ? strength[1] / 2 : 0);
        _clock += _dt;
        const int rows = m ? riseCells(settings.mirror) : stripCells(), cols = m ? W : 1;
        // Every step cools each cell by up to `cool`: the taller the flames, the less.
        const int cool = (20 + (255 - P(1)) * 90 / 255) * 10 / rows + 2, base = m ? 2 : maxi(2, rows / 10);
        const int32_t lean = 64 + ((P(4) - 128) >> 1), blend = P(5); // left-neighbour weight of 128
        const int32_t ember = (P(7) * level) >> 12;
        for (int s = 0; _clock >= step && s < 6; s++) {
            _clock -= step;
            int32_t lv[MAX_BANDS];
            if (m && fuel == 0) for (int i = 0; i < n; i++) lv[i] = level01(autolevel(i, _host->band(i), step));
            const int32_t bass = fuel == 1 ? level01(autolevel(0, _host->level(0), step)) : 0;
            for (int i = 0; i < rows * cols; i++) _v[i] = uint8_t(maxi(0, _v[i] - random8() % cool));
            // Rise from the top row down, so each cell takes the rows below as they were.
            for (int r = rows - 1; r >= 2; r--) {
                if (!m) { _v[r] = uint8_t((_v[r - 1] + 2 * _v[r - 2]) / 3); continue; }
                const int b = (r - 1) * cols;
                for (int c = 0; c < cols; c++) {
                    const int l = c > 0 ? c - 1 : c, rr = c < cols - 1 ? c + 1 : c;
                    const int32_t below = (_v[b + l] * lean + _v[b + c] * 128 + _v[b + rr] * (128 - lean)) >> 8;
                    _v[r * cols + c] = uint8_t((below * 3 + _v[(r - 2) * cols + c]) >> 2);
                }
            }
            // Ignite at the base, keep the embers glowing, flare on hits.
            for (int c = 0; c < cols; c++) {
                const int32_t feed = fuel == 0 ? (m ? cellOf([&](int i) { return lv[i]; }, n, c, cols) : level)
                                   : fuel == 1 ? bass : fuel == 2 ? level : 0;
                if (feed > 123 && random8() < mini(255, (P(2) * (24 + ((232 * feed) >> 12))) >> 7)) {
                    const int at = random(mini(rows, base));
                    _v[at * cols + c] = u8(_v[at * cols + c] + ((((m ? 80 : 120) + random8() / 2) * (1638 + ((2458 * feed) >> 12))) >> 12));
                }
                if (ember > 0) {
                    const int32_t e = ember - random8() % (ember / 2 + 1);
                    if (_v[c] < e) _v[c] = uint8_t(e);
                }
                if (flare > 0)
                    for (int r = 0; r < mini(rows, m ? 2 : 3); r++)
                        _v[r * cols + c] = u8(_v[r * cols + c] + (((m ? 90 : 140) * P(3) / 140 * flare) >> 12));
            }
            // Blend: the base rows smoothed across, so the band flames burn as one fire.
            if (m && blend > 0)
                for (int r = 0; r < mini(rows, base + 1); r++) {
                    uint8_t *row = _v + r * cols;
                    int32_t prev = row[0];
                    for (int c = 0; c < cols; c++) {
                        const int32_t cur = row[c], next = c < cols - 1 ? row[c + 1] : cur;
                        row[c] = uint8_t(cur + (((((prev + next) >> 1) - cur) * blend) >> 8));
                        prev = cur;
                    }
                }
        }
        const int colors = P(6) % 3;
        const int32_t tone = (_tone * 255) >> 12;
        auto flame = [&](int x, int y, int i) {
            const int heat = _v[i];
            if (colors == 0) {
                uint8_t r, g, b;
                heatColor(u8(heat * 5 / 4), r, g, b);
                _host->rgb(x, y, r, g, b);
            } else if (colors == 1) put(x, y, u8(heat * 3 / 2), heat * 215 / 255);
            else put(x, y, u8(heat * 2), tone + heat * 40 / 255);
        };
        if (!m) {
            for (int x = 0; x < W; x++) {
                const int c = stripCell(x);
                if (c < 0) { stripPut(x, 0, 0); continue; }
                for (int y = 0; y < H; y++) flame(x, y, c);
            }
            return;
        }
        for (int x = 0; x < W; x++) {
            const int xp = _host->originX(x);
            for (int y = 0; y < H; y++) {
                if (xp < 0) { put(x, y, 0, 0); continue; }
                flame(x, y, riseCell(y, settings.mirror) * cols + xp);
            }
        }
    }
    // Aurora: a slow noise field that grows, brightens and speeds up with the
    // music; colour follows the tone. No auto-level: calm between beats.
    // Parameters: zoom (64 = 3 noise cells across a matrix, 5 along a strip),
    // fullness, colour spread, bass flash, map (0 the level, 1 spectrum: the
    // curtain grows where its band plays, bass on the left / at the start).
    void aurora() {
        using namespace detail;
        const bool m = matrix(), spectral = P(4) % 2;
        _mood = int16_t(approach(_mood, level01(_host->lineSource()), 80, releaseMs()));
        // Time runs (.25 + 2.5 mood) (.3 + 1.4 speed) noise cells per second.
        const int32_t rate = (16384 + ((163840 * _mood) >> 12)) * (307 + 1434 * settings.speed / 100) >> 10;
        _z += uint32_t(rate * _dt / 1000);
        const int32_t flash = (P(3) * 16 * _host->pulse(0)) >> 12, tone = (_tone * 150) >> 12, cells = m ? W : stripCells();
        const uint32_t zoom = uint32_t(maxi(4, P(0)));
        const uint32_t fx = (m ? 196608u * zoom / 64 / uint32_t(maxi(W, H)) : 327680u * zoom / 64 / uint32_t(maxi(1, cells))), fy = fx * 8 / 5;
        int32_t lv[MAX_BANDS];
        const int n = spectral ? spectrum(lv) : 0;
        for (int x = 0; x < W; x++) {
            const int c = m ? _host->originX(x) : stripCell(x);
            // The curtain's strength here: the level, or the band of this column.
            const int32_t here = c < 0 ? 0 : spectral ? spectrumAt(lv, n, c * ONE / maxi(1, cells - 1)) : _mood;
            const int32_t threshold = P(1) * 16 - ((1229 * here) >> 12);
            const int32_t gate = mini(ONE, here * 10), amp = 1843 + ((2253 * here) >> 12);
            for (int y = 0; y < H; y++) {
                if (c < 0) { put(x, y, 0, 0); continue; }
                // Strips use 2-D noise (position, time): a fixed y = 0 plane of
                // 3-D noise sits on the lattice and barely varies.
                const int32_t nz = (m ? _host->noise(uint32_t(c) * fx, uint32_t(y) * fy + 19712, _z)
                                      : _host->noise(uint32_t(c) * fx, _z)) >> 4;
                const int32_t v = ((clampi((nz - threshold) * 3, 0, ONE) * amp >> 12) + (nz > threshold ? flash : 0)) * gate >> 12;
                put(x, y, u8q(v), (spectral ? bandHue(c, cells) - 20 : tone) + ((nz * P(2)) >> 12));
            }
        }
    }
    // Radial spectrum: matrix = a rotating spectrum sun, strip / ring = a
    // symmetric spectrum that rolls along.
    // Parameters: spin (128 = x1), bass core size.
    void radialSpectrum() {
        using namespace detail;
        const bool m = matrix();
        const int count = mini(MAX_BANDS, m ? maxi(8, mini(W, H) / 2) : maxi(4, stripCells() / 2));
        for (int c = 0; c < count; c++) meter(c, level01(autolevel(c, bandCell(c, count), _dt)));
        const int32_t level = overall(_host->lineSource());
        _phase[0] = advance(_phase[0], ((1311 + ((16384 * level) >> 12)) * (307 + 1434 * settings.speed / 100) >> 10) * P(0) >> 7);
        // Band position t (Q16): 0 = bass .. 1 = treble, mirrored so there is no seam.
        auto bandAt = [&](uint16_t turns, int32_t &t) { t = absi(2 * int32_t(turns) - 65536); return int32_t(_level[mini(count - 1, (t * count) >> 16)]); };
        if (!m) {
            for (int x = 0; x < W; x++) {
                const int xp = _host->originX(x);
                if (xp < 0) { stripPut(x, 0, 0); continue; }
                int32_t t;
                const int32_t l = bandAt(uint16_t(xp * 65536 / W + _phase[0]), t);
                stripPut(x, u8q(l), (t * 255) >> 16);
            }
            return;
        }
        const int32_t cx = W * 8 - 8, cy = radial() ? row16() : H * 8 - 8, rmax = maxi(16, mini(W, H) * 8);
        for (int x = 0; x < W; x++) {
            const int xp = _host->originX(x);
            for (int y = 0; y < H; y++) {
                if (xp < 0) { put(x, y, 0, 0); continue; }
                const int32_t dx = xp * 16 - cx, dy = y * 16 - cy, d = dist(dx, dy);
                int32_t t;
                const int32_t l = bandAt(uint16_t(angle16(dy, dx) + _phase[0]), t);
                // Ray length follows the band (Q4 cover); a bass-driven core glows in the middle.
                const int32_t ray = clampi((((l * 1075) >> 10) * rmax >> 12) - d + 8, 0, 16);
                const int32_t core = (clampi(((((_level[0] * P(1) * 16) >> 12) * rmax >> 12) - d) / 2 + 8, 0, 16) * _level[0]) >> 4;
                const int32_t rv = (ray * (1843 + ((2253 * l) >> 12))) >> 4;
                put(x, y, u8q(maxi(rv, core)), (ray << 8) >= core ? (t * 255) >> 16 : 0);
            }
        }
    }
    // Blobs: a lava lamp of four blobs (bass, bass, mid, treble) that swell
    // with their band and merge.
    // Parameters: blob size (128 = x1), count (1..4), softness of the edges.
    void blobs() {
        using namespace detail;
        const bool m = matrix();
        _mood = int16_t(approach(_mood, overall(_host->lineSource()), 50, releaseMs()));
        static constexpr int groups[4] = {0, 0, 1, 2};
        int32_t bx[4], by[4], br2[4], bh[4], group[3];
        int shift[4];
        for (int g = 0; g < 3; g++) group[g] = level01(autolevel(g, _host->level(g), _dt));
        const int32_t span = m ? mini(W, H) : stripCells(), len = (m ? W : stripCells()) - 1;
        const int32_t speed = 307 + 1741 * settings.speed / 100, pace = 512 + (_mood >> 2); // Q10
        const int count = clampi(P(1), 1, 4);
        for (int k = 0; k < count; k++) {
            const int g = groups[k];
            const int32_t rate = (((3277 + 1507 * k) * speed >> 10) * pace) >> 10; // (.05 + .023k) turns/s
            _phase[k] = advance(_phase[k], rate);
            _phase2[k] = advance(_phase2[k], rate * 1341 >> 10);                    // x 1.31
            _grow[k] = int16_t(approach(_grow[k], group[g] + ((2458 * _host->pulse(g)) >> 12), 30, releaseMs()));
            const int32_t r = (span * 16 * ((m ? 205 + ((655 * _grow[k]) >> 12) : 82 + ((328 * _grow[k]) >> 12)) * P(0) >> 7)) >> 12; // Q4
            br2[k] = r * r;
            shift[k] = 0;
            while ((br2[k] >> shift[k]) >= (1 << 19)) shift[k]++; // keeps br2 << 12 inside 32 bits
            // Centre .5 +- .45 of the lamp (Q4).
            bx[k] = ((32768 + (int32_t(_host->sin16(uint16_t(_phase[k] + k * 16384))) * 29491 >> 15)) * len) >> 12;
            by[k] = ((32768 + (int32_t(_host->sin16(uint16_t(_phase2[k] + k * 26214))) * 29491 >> 15)) * (H - 1)) >> 12;
            bh[k] = g * 85 + 20;
        }
        const int32_t gate = mini(ONE, _mood * 4);
        for (int x = 0; x < W; x++) {
            const int c = m ? _host->originX(x) : stripCell(x);
            for (int y = 0; y < H; y++) {
                if (c < 0) { put(x, y, 0, 0); continue; }
                int32_t field = 0, hue = 0;
                for (int k = 0; k < count; k++) {
                    // Q12 r^2 / (d^2 + 0.3), capped at 16.
                    const int32_t dx = c * 16 - bx[k], dy = m ? y * 16 - by[k] : 0;
                    const uint32_t d2 = uint32_t(dx * dx) + uint32_t(dy * dy) + 77;
                    const int32_t f = mini(65536, int32_t(((uint32_t(br2[k]) >> shift[k]) << 12) / uint32_t(maxi(1, int32_t(d2 >> shift[k])))));
                    field += f; hue += f * bh[k];
                }
                put(x, y, u8q((clampi((field - 2867) * 205 / (P(2) + 1), 0, ONE) * gate) >> 12), hue / maxi(1, field));
            }
        }
    }
    // Field: a moving pattern over the whole lamp. Parameters: pattern (0
    // clouds, 1 plasma, 2 rings, 3 stripes, 4 spiral), zoom (100 = about 3.4
    // waves across), flow (its speed), map (0 the level lights the pattern's
    // crests; 1 spectrum: each value of the pattern is a band, so every part
    // of it lights with its own frequency, bass red .. treble blue), drive (0
    // loudness, 1 bass, 2 mid, 3 treble, 4 beats: what pumps the motion and
    // kicks it), pump (the drive speeds the flow up and swells the zoom), kick
    // (its hits push the pattern on and flash it), contrast (dims the weaker
    // parts), colours (palette spread over the pattern), drift (colours roll), brightness.
    void field() {
        using namespace detail;
        const bool m = matrix(), spectral = P(3) % 2;
        const int pattern = P(0) % 5, drive = P(4) % 5;
        int32_t strength[4] = {0, 0, 0, 0};
        const int mask = hits(strength);
        const int32_t level = drive == 0 ? overall(_host->lineSource())
                            : drive < 4 ? level01(autolevel(drive - 1, _host->level(drive - 1), _dt)) : _host->beat();
        _mood = int16_t(approach(_mood, level, 30, 150));
        int g = 0;
        const int32_t kick = fired(drive == 0 ? 3 : drive == 4 ? 4 : drive - 1, mask, strength, g);
        _boost = int16_t(kick > _boost ? kick : approach(_boost, 0, 1, 250));
        const int32_t pump = (P(5) * _mood) >> 12; // 0..255
        // Flow up to 1 wave per second (x .3 .. 1.7 speed), pumped up to x4, kicked up to +2 waves per second.
        int32_t rate = (P(2) * 256 * (307 + 1434 * settings.speed / 100)) >> 10;
        rate = ((rate * (256 + pump * 3)) >> 8) + ((_boost * P(6)) >> 3);
        _z += uint32_t(rate * _dt / 1000);
        _phase[0] = advance(_phase[0], rate);
        _phase[1] = advance(_phase[1], rate * 5 / 4);
        _phase[2] = advance(_phase[2], rate * 3 / 4);
        _phase[3] = advance(_phase[3], rate / 16);   // stripes turn
        _phase[4] = advance(_phase[4], P(9) * 52);   // colours roll, up to .2 turn per second
        const int cells = m ? W : stripCells();
        const int32_t size = m ? maxi(W, H) : cells;
        // Phase per 1/16 cell (Q16 turns, also noise units): (8 + zoom) / 32 waves across, the pump swells it up to 1.5x.
        const int32_t k = ((128 * (8 + P(1)) / maxi(1, size)) * (1024 + pump * 2)) >> 10;
        const int32_t kx = (k * _host->sin16(uint16_t(_phase[3] + 16384 + 2731))) >> 15, ky = (k * _host->sin16(uint16_t(_phase[3] + 2731))) >> 15;
        const int arms = 1 + (P(1) >> 6);
        const int32_t cx = m ? (W - 1) * 8 : settings.mirror ? 0 : (cells - 1) * 8, cy = radial() ? row16() : (H - 1) * 8;
        // The spectrum map: neighbouring values of the pattern must not land on
        // unrelated bands (a speckle), so it reads eight averaged parts of the spectrum.
        int32_t lv[MAX_BANDS], part[8];
        if (spectral) {
            const int n = spectrum(lv);
            for (int i = 0; i < 8; i++) part[i] = cellOf([&](int b) { return lv[b]; }, n, i, 8);
        }
        const int32_t thr = P(7) * 12, flash = (_boost * P(6)) >> 9;
        const int32_t base = (spectral ? 20 : (_tone * 255) >> 12) + (_phase[4] >> 8);
        for (int x = 0; x < W; x++) {
            const int c = m ? _host->originX(x) : stripCell(x);
            const int32_t dx = c * 16 - cx;
            for (int y = 0; y < H; y++) {
                if (c < 0) { put(x, y, 0, 0); continue; }
                const int32_t dy = !m ? 0 : settings.mirror ? riseCell(y, true) * 16 : y * 16 - cy;
                int32_t u; // the pattern here, Q12
                if (pattern == 0) {
                    const uint32_t nx = uint32_t((1 << 24) + dx * k);
                    const int32_t nz = (m ? _host->noise(nx, uint32_t((1 << 24) + dy * k), _z) : _host->noise(nx, _z)) >> 4;
                    u = clampi(2048 + (((nz - 2048) * 3) >> 1), 0, ONE);
                } else if (pattern == 1) {
                    const int32_t d = dist(dx, dy);
                    const int32_t s = _host->sin16(uint16_t(dx * k + _phase[0])) + _host->sin16(uint16_t(dy * k * 13 / 16 - _phase[1] + 21845))
                                    + _host->sin16(uint16_t((dx + dy) * k * 10 / 16 + _phase[2])) + _host->sin16(uint16_t(d * k * 3 / 4 - _phase[0]));
                    u = clampi(2048 + ((s * 3) >> 7), 0, ONE);
                } else if (pattern == 2) u = wave16(uint16_t(dist(dx, dy) * k - _phase[0])) >> 4;
                else if (pattern == 3) u = wave16(uint16_t(dx * kx + dy * ky - _phase[0])) >> 4;
                else u = wave16(uint16_t(int32_t(angle16(dy, dx)) * arms + dist(dx, dy) * k - _phase[0])) >> 4;
                const int32_t raw = spectral ? spectrumAt(part, 8, u) : (u * _mood) >> 12;
                const int32_t v = clampi((raw - thr) * ONE / (ONE - thr), 0, ONE) + ((flash * u) >> 12);
                put(x, y, u8q(v), base + ((u * P(8)) >> 12));
            }
        }
    }
    // Chromatic Flow: soft, slow colour regions, one per sounding pitch class,
    // each at its note's place on the palette: the circle of fifths (C G D A
    // E B F# C# G# D# A# F, so related notes sit side by side) or the
    // semitones in order. A slowly drifting noise field is split by the
    // classes' shares, so a lone note fills the lamp and a chord shares it.
    // Bass hits and beats only add a glow and swell the regions a little.
    // Parameters: order (0 fifths, 1 semitones), shift (where C sits on the
    // palette), follow (the classes glide over 50 + 8v ms), focus (classes
    // under v/256 of the strongest are left out), zoom (80 = 2.5 noise cells
    // across a matrix, 5 along a strip), flow (drift), blend (soft borders),
    // glow on hits, swell on hits, brightness.
    void chromaticFlow() {
        using namespace detail;
        const bool m = matrix();
        const int32_t follow = 50 + P(2) * 8;
        int32_t now = 0, top = 0;
        for (int i = 0; i < 12; i++) {
            const int32_t c = clampi(_host->chroma(i), 0, ONE);
            now = maxi(now, c);
            _level[i] = int16_t(approach(_level[i], c, follow, follow));
            top = maxi(top, _level[i]);
        }
        // The light follows the strongest class now: the stream's chroma falls in a fade or a pause.
        _mood = int16_t(approach(_mood, level01(now), 80, follow + 200));
        int32_t strength[4] = {0, 0, 0, 0};
        const int mask = hits(strength);
        const int32_t kick = mask & 1 ? strength[0] : mask & 8 ? strength[3] : 0;
        _boost = int16_t(kick > _boost ? kick : approach(_boost, 0, 1, 400));
        // Drift: up to one noise cell per second (x .3 .. 1.7 motion speed).
        _z += uint32_t(((P(5) * 256 * (307 + 1434 * settings.speed / 100)) >> 10) * _dt / 1000);
        // Each class's share of the noise range (Q12), in palette order.
        const int32_t gate = (top * P(3)) >> 8;
        int32_t edge[13], hue[12], total = 0;
        int count = 0;
        edge[0] = 0;
        for (int k = 0; k < 12; k++) {
            const int32_t w = _level[P(0) % 2 ? k : k * 7 % 12] - gate;
            if (w <= 0) continue;
            total += w;
            edge[++count] = total;
            hue[count - 1] = (k * 64 / 3 + P(1)) & 255;
        }
        for (int j = 1; j <= count; j++) edge[j] = edge[j] * ONE / total;
        const int cells = m ? W : stripCells(), rows = m ? H : 1;
        const int32_t size = m ? maxi(W, H) : cells;
        // Noise units per 1/16 LED; a hit swells the regions by up to a quarter.
        const int32_t swell = (_boost * P(8)) >> 10;
        const uint32_t k = ((uint32_t(m ? 8192 : 16384) * uint32_t(maxi(4, P(4))) / 64 / uint32_t(maxi(1, size))) * uint32_t(ONE - swell)) >> 12;
        const int32_t cx = m ? (W - 1) * 8 : settings.mirror ? 0 : (cells - 1) * 8, cy = radial() ? row16() : (H - 1) * 8;
        // The noise of every lit place (8 bits, in _v) and its rank on the lamp
        // (Q12, ties at their middle): the classes get their shares of the lamp
        // whatever the noise's spread.
        uint16_t rank[256] = {};
        int lit = 0;
        for (int x = 0; x < W; x++) {
            const int c = m ? _host->originX(x) : stripCell(x);
            if (c < 0 || !count) continue;
            const uint32_t nx = uint32_t((1 << 24) + (c * 16 - cx) * int32_t(k));
            for (int y = 0; y < rows; y++) {
                const int32_t dy = settings.mirror ? riseCell(y, true) * 16 : y * 16 - cy;
                const uint8_t nz = uint8_t((m ? _host->noise(nx, uint32_t((1 << 24) + dy * int32_t(k)), _z) : _host->noise(nx, _z)) >> 8);
                _v[y * W + x] = nz;
                rank[nz]++; lit++;
            }
        }
        for (int32_t v = 0, below = 0; v < 256; v++) {
            const int32_t same = rank[v];
            rank[v] = uint16_t((2 * below + same) * ONE / maxi(1, 2 * lit));
            below += same;
        }
        // 70% of the light, so a hit's glow (up to x2) shows.
        const int32_t base = (_mood * 2867) >> 12, blend = P(6) * 4, glow = (_boost * P(7)) >> 8;
        for (int x = 0; x < W; x++) {
            const int c = m ? _host->originX(x) : stripCell(x);
            for (int y = 0; y < H; y++) {
                if (c < 0 || !count) { put(x, y, 0, 0); continue; }
                const int32_t u = rank[_v[m ? y * W + x : x]];
                int j = 0;
                while (u >= edge[j + 1]) j++;
                // Near a border the colour turns towards the neighbour's (halfway at the border) and dims a little.
                int32_t h = hue[j], v = base;
                const int32_t up = j + 1 < count ? edge[j + 1] - u : ONE, down = j ? u - edge[j] : ONE, d = mini(up, down);
                if (d < blend) {
                    const int32_t other = hue[up <= down ? j + 1 : j - 1], dh = ((other - h + 128) & 255) - 128;
                    h += dh * (blend - d) / (2 * blend);
                    v -= (v * (blend - d) / blend) >> 2;
                }
                put(x, y, u8q(v + ((v * glow) >> 12)), h & 255);
            }
        }
    }
#endif
};
} // namespace Viz
