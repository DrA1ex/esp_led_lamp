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
//   int bandCount(); int band(int i); int wave(int index, int span);  (Q12)
//   uint8_t random8(); int16_t sin16(uint16_t);
//   uint16_t noise(uint32_t x, uint32_t y[, uint32_t z]);  (inoise16)
//   void put(int x, int y, uint8_t v, uint8_t hue);
//
// Strips (HEIGHT < 4, every row the same) run along the LEDs from the origin,
// or out from the centre when mirrored; matrices use the height and mirror
// vertically. y = 0 is the bottom row.
#include <stdint.h>

#ifndef VIZ_WITH_HEAVY
#define VIZ_WITH_HEAVY 1
#endif

namespace Viz {
// Lab audio mode = 13 + id, firmware AudioEffectEnum = 12 + id.
enum Mode : uint8_t {
    VU_METER, SPECTRUM_BARS, FLOW, PULSES, SPARKLES, MOOD, WAVEFORM, BAND_METERS,
    FIRE, AURORA, JUGGLE, RADIAL_SPECTRUM, BLOBS, MODE_COUNT
};
// Per-pixel noise, angles and fields: compiled only with VIZ_WITH_HEAVY.
inline bool heavy(uint8_t m) { return m == FIRE || m == AURORA || m == RADIAL_SPECTRUM || m == BLOBS; }
inline bool available(uint8_t m) { return m < MODE_COUNT && (VIZ_WITH_HEAVY || !heavy(m)); }

constexpr int32_t ONE = 4096; // Q12 1.0

struct Settings {
    bool mirror = false;    // strips: from the centre both ways; matrices: from the middle row
    bool autolevel = true;  // each band / meter stretched over its own recent range
    uint8_t decay = 50;     // 0..100 %: meter release and trail length
    uint8_t speed = 50;     // 0..100 %: motion speed (Flow, Pulses and the free-form modes)
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
} // namespace detail

template<class Host, int MAX_PIX>
class Engine {
    static constexpr int MAX_BANDS = 64, CHANNELS = MAX_BANDS + 1, OVERALL = MAX_BANDS;
    struct Comet { int16_t pos, energy; uint8_t hue; bool on; }; // pos, energy Q12

    int16_t _level[CHANNELS], _peak[CHANNELS], _hold[CHANNELS]; // Q12, Q12, ms
    int32_t _lo[CHANNELS], _hi[CHANNELS];                       // auto-level range, Q20
    uint8_t _v[MAX_PIX], _h[MAX_PIX];
    int16_t _prev_pulse[3], _cool[4], _tone = 0, _mood = 0, _grow[4];
    int32_t _clock = 0, _spawn = 0; // ms, Q8 sparks
    uint16_t _phase[8], _phase2[8]; // Q16 turns
    uint32_t _z = 0;                // Aurora time, noise units
    Comet _comets[24];
    int32_t _key = -1;
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
#if VIZ_WITH_HEAVY
            case FIRE: fire(); break;
            case AURORA: aurora(); break;
            case RADIAL_SPECTRUM: radialSpectrum(); break;
            case BLOBS: blobs(); break;
#endif
            default: break;
        }
    }

private:
    // ---- shared helpers -------------------------------------------------
    void reset(uint8_t mode) {
        for (int i = 0; i < CHANNELS; i++) { _level[i] = _peak[i] = _hold[i] = 0; _lo[i] = _hi[i] = 0; }
        for (int i = 0; i < MAX_PIX; i++) _v[i] = _h[i] = 0;
        for (Comet &c: _comets) c.on = false;
        for (int g = 0; g < 3; g++) _prev_pulse[g] = 0;
        for (int k = 0; k < 4; k++) _cool[k] = _grow[k] = 0;
        // Start phases k * 0.13 turn; the second axis runs 1.37x (Juggle) or 1.31x (Blobs) faster.
        for (int k = 0; k < 8; k++) { _phase[k] = uint16_t(k * 8520); _phase2[k] = uint16_t(k * (mode == BLOBS ? 11161 : 11672)); }
        _clock = _spawn = 0; _mood = 0; _z = 0;
        _tone = int16_t(toneNow());
    }
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
    void put(int x, int y, uint8_t v, int32_t hue) { _host->put(x, y, v, detail::u8(hue)); }
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
    void fade(int n, int32_t tau) {
        using namespace detail;
        const uint32_t keep = uint32_t(255 - mini(255, 255 * _dt / maxi(1, tau) + 1));
        for (int i = 0; i < n; i++) { const uint32_t x = _v[i] * keep; _v[i] = uint8_t((x + 1 + (x >> 8)) >> 8); }
    }
    void stamp(int i, uint8_t v, int32_t hue) { if (v >= _v[i]) { _v[i] = v; _h[i] = detail::u8(hue); } }

    // ---- modes -------------------------------------------------------------
    // VU meter: strip fill from the origin (or both ways), matrix fill from the
    // bottom (or the middle, or a circle with the radial origin).
    void vuMeter() {
        using namespace detail;
        meter(0, level01(autolevel(0, _host->lineSource(), _dt)));
        if (!matrix()) {
            const int cells = stripCells();
            for (int x = 0; x < W; x++) {
                const int c = stripCell(x);
                stripPut(x, c < 0 ? 0 : meterV(c, cells, _level[0], _peak[0]), c * 230 / maxi(1, cells - 1));
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
                put(x, y, meterV(c, cells, _level[0], _peak[0]), c * 230 / maxi(1, cells - 1));
            }
        }
    }
    // Spectrum: strip = smooth bands along the LEDs, matrix = bars with
    // falling peaks. Meters run per band and are spread over the cells.
    void spectrumBars() {
        using namespace detail;
        const int n = bands();
        for (int i = 0; i < n; i++) meter(i, level01(autolevel(i, _host->band(i), _dt)));
        auto level = [&](int c, int cells) { return cellOf([&](int k) { return int32_t(_level[k]); }, n, c, cells); };
        if (!matrix()) {
            const int cells = stripCells();
            for (int x = 0; x < W; x++) {
                const int c = stripCell(x);
                stripPut(x, c < 0 ? 0 : u8q(level(c, cells)), c * 255 / maxi(1, cells - 1));
            }
            return;
        }
        const int rows = riseCells(settings.mirror);
        for (int x = 0; x < W; x++) {
            const int xp = _host->originX(x);
            const int32_t l = xp < 0 ? 0 : level(xp, W), p = xp < 0 ? 0 : cellOf([&](int k) { return int32_t(_peak[k]); }, n, xp, W);
            for (int y = 0; y < H; y++) {
                if (xp < 0) { put(x, y, 0, 0); continue; }
                const int r = riseCell(y, settings.mirror);
                put(x, y, meterV(r, rows, l, p), r * 255 / maxi(1, rows - 1));
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
    void pulses() {
        using namespace detail;
        const bool m = matrix();
        const int32_t line = level01(_host->lineSource());
        int32_t strength[3] = {0, 0, 0};
        const int mask = onsets(strength);
        for (int g = 0; g < 3; g++)
            if (mask & (1 << g)) {
                Comet *slot = &_comets[0];
                for (Comet &c: _comets) { if (!c.on) { slot = &c; break; } if (c.energy < slot->energy) slot = &c; }
                *slot = {0, int16_t(mini(ONE, (strength[g] * (2253 + ((1843 * line) >> 12))) >> 12)), uint8_t(g * 85 + 20), true};
            }
        const int n = m ? W * H : stripCells();
        fade(n, releaseMs() * 3 / 10);
        // Rings: radius pos (ONE = half the diagonal), 1.5 LEDs (24 / 16) thick.
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
                const int32_t lo = maxi(0, r0 - 24), hi = r1 + 24;
                const uint32_t lo2 = uint32_t(lo * lo), hi2 = uint32_t(hi * hi);
                for (int y = 0; y < H; y++)
                    for (int x = 0; x < W; x++) {
                        const int32_t dx = x * 16 - cx, dy = y * 16 - cy;
                        const uint32_t d2 = uint32_t(dx * dx) + uint32_t(dy * dy);
                        if (d2 < lo2 || d2 > hi2) continue;
                        const int32_t r = int32_t(isqrt(d2)), d = r < r0 ? r0 - r : (r > r1 ? r - r1 : 0);
                        if (d < 24) stamp(y * W + x, u8q(e * (24 - d) / 24), c.hue);
                    }
            }
            if (c.pos >= ONE) c.on = false;
        }
        const uint8_t glow = uint8_t((31 * line) >> 12);
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
    void sparkles() {
        using namespace detail;
        const int n = W * H;
        fade(n, releaseMs() * 6 / 10);
        int32_t strength[3] = {0, 0, 0};
        const int mask = onsets(strength);
        _spawn += _dt * n * (5 + ((307 * level01(_host->level(2))) >> 12)) / 1000;
        for (int g = 0; g < 3; g++) if (mask & (1 << g)) _spawn += (n * 13 * strength[g]) >> 12;
        for (int k = 0; k < n && _spawn >= 256; k++, _spawn -= 256) {
            const int g = mask & 4 ? 2 : (mask & 2 ? 1 : (mask & 1 ? 0 : 2));
            stamp(random(n), uint8_t(191 + random8() / 4), g * 85 + random8() / 8);
        }
        _spawn = mini(_spawn, n << 8);
        const uint8_t glow = uint8_t((89 * level01(_host->level(0))) >> 12);
        const int32_t tone = (_tone * 120) >> 12;
        for (int i = 0, y = 0; y < H; y++)
            for (int x = 0; x < W; x++, i++) put(x, y, _v[i] > glow ? _v[i] : glow, _v[i] > glow ? _h[i] : tone);
    }
    // Mood: the whole lamp breathes with the level, colour follows the tone.
    void mood() {
        using namespace detail;
        _mood = int16_t(approach(_mood, level01(_host->control()), 50, releaseMs()));
        const uint8_t v = u8q(_mood + ((1434 * _host->pulse(0)) >> 12));
        const int32_t hue = (_tone * 200) >> 12, cells = stripCells();
        for (int y = 0; y < H; y++)
            for (int x = 0; x < W; x++) {
                const int sc = matrix() ? 0 : stripCell(x);
                const int32_t c = matrix() ? (x + y) * 48 / maxi(1, W + H - 2) : (sc < 0 ? 0 : sc * 48 / maxi(1, cells - 1));
                put(x, y, v, hue + c);
            }
    }
    // Waveform: strip = coloured loudness history, matrix = history as bars / mirrored waveform.
    void waveform() {
        using namespace detail;
        if (!matrix()) {
            const int cells = stripCells();
            for (int x = 0; x < W; x++) {
                const int c = stripCell(x);
                const uint8_t v = c < 0 ? 0 : u8q(level01(_host->wave(c, cells)));
                stripPut(x, v, v);
            }
            return;
        }
        const int rows = riseCells(settings.mirror);
        for (int x = 0; x < W; x++) {
            const int xp = _host->originX(x);
            const int32_t a = xp < 0 ? 0 : level01(_host->wave(xp, W)) * rows;
            for (int y = 0; y < H; y++) {
                const int r = riseCell(y, settings.mirror);
                put(x, y, xp < 0 ? 0 : u8((a - (int32_t(r) << 12)) >> 4), r * 255 / maxi(1, rows - 1));
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
    void juggle() {
        using namespace detail;
        const bool m = matrix();
        const int n = m ? W * H : stripCells(), dots = 8;
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
                const int32_t pos = (wave16(_phase[k]) * (n - 1)) >> 8, half = 154 + (b >> 3);
                for (int i = maxi(0, (pos - half) >> 8); i <= mini(n - 1, ((pos + half) >> 8) + 1); i++)
                    stamp(i, u8(((b >> 4) * clampi(half - absi((i << 8) - pos) + 128, 0, 256)) >> 8), hue);
            } else {
                // Lissajous paths (Q4 positions), the dot grows with its band.
                const int32_t px = (wave16(_phase[k]) * (W - 1)) >> 12;
                const int32_t py = (wave16(uint16_t(_phase2[k] + k * 7209)) * (H - 1)) >> 12;
                const int32_t rad = 11 + ((b * maxi(16, mini(W, H) * 16 / 10)) >> 12);
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

#if VIZ_WITH_HEAVY
    // Fire: bass and onsets feed the flames; strip burns from the start (or
    // centre), matrix from the bottom with a flame per band.
    void fire() {
        using namespace detail;
        const bool m = matrix();
        const int32_t step = 33 - 22 * settings.speed / 100, level = overall(_host->lineSource());
        const int cooling = 90 - settings.decay * 60 / 100, n = bands();
        int32_t strength[3] = {0, 0, 0};
        const int mask = onsets(strength);
        _clock += _dt;
        const int rows = m ? riseCells(settings.mirror) : stripCells(), cols = m ? W : 1;
        for (int s = 0; _clock >= step && s < 6; s++) {
            _clock -= step;
            int32_t lv[MAX_BANDS];
            if (m) for (int i = 0; i < n; i++) lv[i] = level01(autolevel(i, _host->band(i), step));
            for (int c = 0; c < cols; c++) {
                // Cool, rise (each cell takes from the cells below), ignite at the base.
                for (int r = 0; r < rows; r++) {
                    const int i = r * cols + c, cool = random8() % ((cooling * 10) / rows + 2);
                    _v[i] = uint8_t(maxi(0, _v[i] - cool));
                }
                for (int r = rows - 1; r >= 2; r--) {
                    if (!m) { _v[r] = uint8_t((_v[r - 1] + 2 * _v[r - 2]) / 3); continue; }
                    const int l = maxi(0, c - 1), rr = mini(cols - 1, c + 1), b = (r - 1) * cols;
                    _v[r * cols + c] = uint8_t((_v[b + l] + _v[b + c] + _v[b + rr] + _v[(r - 2) * cols + c]) / 4);
                }
                const int32_t feed = m ? cellOf([&](int i) { return lv[i]; }, n, c, cols) : level;
                const int32_t boost = (mask & 1 ? strength[0] : 0) + (mask & 2 ? strength[1] / 2 : 0);
                if (feed > 123 && random8() < 20 + ((220 * feed) >> 12)) {
                    const int at = random(mini(rows, m ? 2 : maxi(2, rows / 10)));
                    _v[at * cols + c] = u8(_v[at * cols + c] + ((((m ? 80 : 120) + random8() / 2) * (1638 + ((2458 * feed) >> 12))) >> 12));
                }
                if (boost > 0)
                    for (int r = 0; r < mini(rows, m ? 2 : 3); r++)
                        _v[r * cols + c] = u8(_v[r * cols + c] + (((m ? 90 : 140) * boost) >> 12));
            }
        }
        auto flame = [&](int x, int y, int i) { const int heat = _v[i]; put(x, y, u8(heat * 3 / 2), heat * 215 / 255); };
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
    void aurora() {
        using namespace detail;
        const bool m = matrix();
        _mood = int16_t(approach(_mood, level01(_host->lineSource()), 80, releaseMs()));
        // Time runs (.25 + 2.5 mood) (.3 + 1.4 speed) noise cells per second.
        const int32_t rate = (16384 + ((163840 * _mood) >> 12)) * (307 + 1434 * settings.speed / 100) >> 10;
        _z += uint32_t(rate * _dt / 1000);
        const int32_t threshold = 2048 - ((1229 * _mood) >> 12), flash = (1229 * _host->pulse(0)) >> 12;
        const int32_t gate = mini(ONE, _mood * 10), amp = 1843 + ((2253 * _mood) >> 12), tone = (_tone * 150) >> 12;
        // Noise cells along the lamp: 3 across a matrix, 5 along a strip.
        const uint32_t fx = m ? 196608u / uint32_t(maxi(W, H)) : 327680u / uint32_t(maxi(1, stripCells())), fy = fx * 8 / 5;
        for (int x = 0; x < W; x++) {
            const int c = m ? _host->originX(x) : stripCell(x);
            for (int y = 0; y < H; y++) {
                if (c < 0) { put(x, y, 0, 0); continue; }
                // Strips use 2-D noise (position, time): a fixed y = 0 plane of
                // 3-D noise sits on the lattice and barely varies.
                const int32_t nz = (m ? _host->noise(uint32_t(c) * fx, uint32_t(y) * fy + 19712, _z)
                                      : _host->noise(uint32_t(c) * fx, _z)) >> 4;
                const int32_t v = ((clampi((nz - threshold) * 3, 0, ONE) * amp >> 12) + (nz > threshold ? flash : 0)) * gate >> 12;
                put(x, y, u8q(v), tone + ((nz * 90) >> 12));
            }
        }
    }
    // Radial spectrum: matrix = a rotating spectrum sun, strip / ring = a
    // symmetric spectrum that rolls along.
    void radialSpectrum() {
        using namespace detail;
        const bool m = matrix();
        const int count = mini(MAX_BANDS, m ? maxi(8, mini(W, H) / 2) : maxi(4, stripCells() / 2));
        for (int c = 0; c < count; c++) meter(c, level01(autolevel(c, bandCell(c, count), _dt)));
        const int32_t level = overall(_host->lineSource());
        _phase[0] = advance(_phase[0], (1311 + ((16384 * level) >> 12)) * (307 + 1434 * settings.speed / 100) >> 10);
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
                const int32_t core = (clampi(((((_level[0] * 1434) >> 12) * rmax >> 12) - d) / 2 + 8, 0, 16) * _level[0]) >> 4;
                const int32_t rv = (ray * (1843 + ((2253 * l) >> 12))) >> 4;
                put(x, y, u8q(maxi(rv, core)), (ray << 8) >= core ? (t * 255) >> 16 : 0);
            }
        }
    }
    // Blobs: a lava lamp of four blobs (bass, bass, mid, treble) that swell
    // with their band and merge.
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
        for (int k = 0; k < 4; k++) {
            const int g = groups[k];
            const int32_t rate = (((3277 + 1507 * k) * speed >> 10) * pace) >> 10; // (.05 + .023k) turns/s
            _phase[k] = advance(_phase[k], rate);
            _phase2[k] = advance(_phase2[k], rate * 1341 >> 10);                    // x 1.31
            _grow[k] = int16_t(approach(_grow[k], group[g] + ((2458 * _host->pulse(g)) >> 12), 30, releaseMs()));
            const int32_t r = (span * 16 * (m ? 205 + ((655 * _grow[k]) >> 12) : 82 + ((328 * _grow[k]) >> 12))) >> 12; // Q4
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
                for (int k = 0; k < 4; k++) {
                    // Q12 r^2 / (d^2 + 0.3), capped at 16.
                    const int32_t dx = c * 16 - bx[k], dy = m ? y * 16 - by[k] : 0;
                    const uint32_t d2 = uint32_t(dx * dx) + uint32_t(dy * dy) + 77;
                    const int32_t f = mini(65536, int32_t(((uint32_t(br2[k]) >> shift[k]) << 12) / uint32_t(maxi(1, int32_t(d2 >> shift[k])))));
                    field += f; hue += f * bh[k];
                }
                put(x, y, u8q((clampi((field - 2867) * 8 / 5, 0, ONE) * gate) >> 12), hue / maxi(1, field));
            }
        }
    }
#endif
};
} // namespace Viz
