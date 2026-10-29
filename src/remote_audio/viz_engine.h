#pragma once
// Geometry-aware audio visualizations shared by the ESP LED Music Lab
// (esp_led_lab/src/viz_engine.h, WebAssembly preview) and the firmware
// (esp_led_lamp/src/remote_audio/viz_engine.h). Keep both copies identical.
//
// Freestanding: no libc / libm. The host supplies the inputs (all 0..1),
// the reaction transfer, random numbers, sin16, noise and the pixel output:
//   int width(), height();  int originX(int x) (-1: dark);
//   bool radial(); float radialRow();  float transfer(float v);
//   float control(), lineSource(), level(int g), pulse(int g);
//   int bandCount(); float band(int i); float wave(int index, int span);
//   uint8_t random8(); int16_t sin16(uint16_t);
//   uint16_t noise(uint32_t x, uint32_t y[, uint32_t z]);  (inoise16)
//   void put(int x, int y, float v, int hue);  (v 0..1, hue 0..255)
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

struct Settings {
    bool mirror = false;    // strips: from the centre both ways; matrices: from the middle row
    bool autolevel = true;  // each band / meter stretched over its own recent range
    float decay = .5f;      // 0..1 meter release and trail length
    float speed = .5f;      // 0..1 motion speed (Flow, Pulses and the free-form modes)
};

namespace detail {
inline float minf(float a, float b) { return a < b ? a : b; }
inline float maxf(float a, float b) { return a > b ? a : b; }
inline int mini(int a, int b) { return a < b ? a : b; }
inline int maxi(int a, int b) { return a > b ? a : b; }
inline float clampf(float v, float l, float h) { return maxf(l, minf(h, v)); }
inline float absf(float v) { return v < 0 ? -v : v; }
inline float fract(float x) { return x - (int) x + (x < 0 && x != (int) x ? 1 : 0); }
inline uint8_t u8(int a) { return (uint8_t) maxi(0, mini(a, 255)); }
inline float sqrtf_(float v) { return __builtin_sqrtf(v); }
// Angle of (x, y) in turns 0..1 (polynomial atan).
inline float angleTurns(float y, float x) {
    const float ax = absf(x), ay = absf(y), a = minf(ax, ay) / maxf(1e-6f, maxf(ax, ay)), s = a * a;
    float r = ((-.0464964749f * s + .15931422f) * s - .327622764f) * s * a + a;
    if (ay > ax) r = 1.57079637f - r;
    if (x < 0) r = 3.14159274f - r;
    if (y < 0) r = 6.28318548f - r;
    return r / 6.28318548f;
}
} // namespace detail

template<class Host, int MAX_PIX>
class Engine {
    static constexpr int MAX_BANDS = 64, CHANNELS = MAX_BANDS + 1, OVERALL = MAX_BANDS;
    struct Comet { float pos, energy; uint8_t hue; bool on; };

    float _level[CHANNELS], _peak[CHANNELS], _hold[CHANNELS], _lo[CHANNELS], _hi[CHANNELS];
    uint8_t _v[MAX_PIX], _h[MAX_PIX];
    float _prev_pulse[3], _cool[4], _clock = 0, _tone = 0, _mood = 0, _spawn = 0, _phase[8];
    Comet _comets[24];
    int32_t _key = -1;
    Host *_host = nullptr;
    int W = 1, H = 1;
    float _dt = 0;

public:
    Settings settings;

    void invalidate() { _key = -1; }

    void render(Host &host, uint8_t mode, float dt) {
        using namespace detail;
        _host = &host; W = host.width(); H = host.height(); _dt = dt;
        if (!available(mode) || W * H > MAX_PIX) return;
        const int32_t key = int32_t(mode) * 100000 + W * 128 + H + (settings.mirror ? 50000 : 0);
        if (key != _key) { _key = key; reset(); }
        _tone = approach(_tone, toneNow(), dt, .15f, .15f);
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
    void reset() {
        for (int i = 0; i < CHANNELS; i++) _level[i] = _peak[i] = _hold[i] = _lo[i] = _hi[i] = 0;
        for (int i = 0; i < MAX_PIX; i++) _v[i] = _h[i] = 0;
        for (Comet &c: _comets) c.on = false;
        for (int g = 0; g < 3; g++) _prev_pulse[g] = 0;
        for (int k = 0; k < 4; k++) _cool[k] = 0;
        for (int k = 0; k < 8; k++) _phase[k] = k * .13f;
        _clock = _mood = _spawn = 0;
        _tone = toneNow();
    }
    bool matrix() const { return H >= 4; }
    int bands() const { const int n = _host->bandCount(); return n > 0 && n <= MAX_BANDS ? n : 16; }
    // Cell i of `cells` from n per-band values: fewer cells than bands average
    // their band range, more cells interpolate (firmware RemoteSignalProvider).
    template<class F>
    static float cellOf(F get, int n, int i, int cells) {
        using namespace detail;
        if (cells <= 1) { float sum = 0; for (int k = 0; k < n; k++) sum += get(k); return sum / n; }
        if (cells < n) {
            const int a = i * n / cells, b = maxi(a + 1, (i + 1) * n / cells);
            float sum = 0; for (int k = a; k < b; k++) sum += get(k);
            return sum / (b - a);
        }
        const float pos = float(i) * (n - 1) / (cells - 1);
        const int a = (int) pos;
        if (a >= n - 1) return get(n - 1);
        const float f = pos - a;
        return get(a) * (1 - f) + get(a + 1) * f;
    }
    float bandCell(int i, int cells) const { return cellOf([&](int k) { return _host->band(k); }, bands(), i, cells); }
    // 0 = bass-heavy .. 1 = treble-heavy.
    float toneNow() const {
        const float b = _host->level(0), m = _host->level(1), h = _host->level(2), sum = b + m + h;
        return sum > .02f ? (m * .5f + h) / sum : _tone;
    }
    static float approach(float cur, float target, float dt, float attack, float release) {
        using namespace detail;
        return cur + (target - cur) * minf(1.f, dt / maxf(1e-3f, target > cur ? attack : release));
    }
    float release() const { return .04f + settings.decay * .7f; }
    float level01(float v) const { return _host->transfer(detail::clampf(v, 0.f, 1.f)); }
    // Auto-level channel i: its own recent range (~3 s) is stretched towards
    // 0..1, so quiet bands and the treble meter move like the bass. Only part
    // of the floor is subtracted, so a steady signal stays up instead of 0.
    float autolevel(int i, float x, float dt) {
        using namespace detail;
        if (!settings.autolevel) return x;
        const float k = minf(1.f, dt / 3.f);
        _hi[i] = x > _hi[i] ? x : _hi[i] + (x - _hi[i]) * k;
        _lo[i] = x < _lo[i] ? x : _lo[i] + (x - _lo[i]) * k;
        const float floor = .8f * _lo[i];
        return clampf((x - floor) / maxf(.1f, _hi[i] - floor), 0.f, 1.f);
    }
    float overall(float line) { return level01(autolevel(OVERALL, line, _dt)); }
    uint8_t random8() { return _host->random8(); }
    int random(int n) { return ((random8() << 8) | random8()) % detail::maxi(1, n); }
    float sinTurns(float turns) const { return _host->sin16(uint16_t((int32_t) (detail::fract(turns) * 65536.f))) / 32767.f; }

    int stripCells() const { return settings.mirror ? W / 2 + 1 : W; }
    int stripCell(int x) const {
        const int xp = _host->originX(x);
        if (xp < 0) return -1;
        return settings.mirror ? (xp <= W / 2 ? W / 2 - xp : xp - W / 2) : xp;
    }
    int riseCells(bool mirror) const { return mirror ? (H + 1) / 2 : H; }
    int riseCell(int y, bool mirror) const { if (!mirror) return y; const int c = H / 2; return y < c ? c - 1 - y : y - c; }
    bool radial() const { return _host->radial() && H > 1; }
    float originMaxDistance() const {
        using namespace detail;
        const float mx = maxi(W / 2, W - 1 - W / 2);
        if (!radial()) return maxf(1.f, mx);
        const float cy = _host->radialRow(), my = maxf(cy, H - 1 - cy);
        return maxf(1.f, sqrtf_(mx * mx + my * my));
    }
    void put(int x, int y, float v, int hue) { _host->put(x, y, detail::clampf(v, 0.f, 1.f), detail::maxi(0, detail::mini(hue, 255))); }
    void stripPut(int x, float v, int hue) { for (int y = 0; y < H; y++) put(x, y, v, hue); }

    void meter(int i, float target) {
        using namespace detail;
        _level[i] = approach(_level[i], target, _dt, .015f, release());
        if (_level[i] >= _peak[i]) { _peak[i] = _level[i]; _hold[i] = .4f; }
        else if (_hold[i] > 0) _hold[i] -= _dt;
        else _peak[i] = maxf(_level[i], _peak[i] - _dt * (.25f + 1.5f * (1.f - settings.decay)));
    }
    // Fill of `cells` up to level with an anti-aliased edge, plus the peak dot.
    static float meterV(int cell, int cells, float level, float peak) {
        using namespace detail;
        const float edge = level * cells;
        float v = clampf(edge - cell, 0.f, 1.f);
        const int pk = mini(cells - 1, (int) (peak * cells));
        if (peak > .02f && cell == pk && pk >= (int) edge) v = 1.f;
        return v;
    }
    // Rising-edge onsets per low / mid / high group. Each group re-triggers
    // after .4 / .32 / .24 s and onsets are at least .15 s apart.
    int onsets(float strength[3]) {
        int mask = 0;
        for (int k = 0; k < 4; k++) _cool[k] -= _dt;
        for (int g = 0; g < 3; g++) {
            const float p = _host->pulse(g);
            if (p > .5f && p > _prev_pulse[g] + .2f && _cool[g] <= 0 && _cool[3] <= 0) {
                mask |= 1 << g; strength[g] = p; _cool[g] = .4f - .08f * g; _cool[3] = .15f;
            }
            _prev_pulse[g] = p;
        }
        return mask;
    }
    void fade(int n, float tau) {
        using namespace detail;
        const int keep = 255 - mini(255, (int) (255.f * _dt / maxf(1e-3f, tau)) + 1);
        for (int i = 0; i < n; i++) _v[i] = uint8_t(_v[i] * keep / 255);
    }
    void stamp(int i, float v, int hue) {
        const uint8_t b = detail::u8((int) (v * 255.f));
        if (b >= _v[i]) { _v[i] = b; _h[i] = uint8_t(hue); }
    }

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
        const int cells = radial() ? maxi(W, H) / 2 + 1 : riseCells(settings.mirror);
        const float maxd = originMaxDistance();
        for (int x = 0; x < W; x++) {
            const int xp = _host->originX(x);
            for (int y = 0; y < H; y++) {
                if (xp < 0) { put(x, y, 0, 0); continue; }
                int c;
                if (radial()) {
                    const float dx = xp - W / 2.f, dy = y - _host->radialRow();
                    c = mini(cells - 1, (int) (sqrtf_(dx * dx + dy * dy) * (cells - 1) / maxd));
                } else c = riseCell(y, settings.mirror);
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
        auto level = [&](int c, int cells) { return cellOf([&](int k) { return _level[k]; }, n, c, cells); };
        if (!matrix()) {
            const int cells = stripCells();
            for (int x = 0; x < W; x++) {
                const int c = stripCell(x);
                stripPut(x, c < 0 ? 0 : level(c, cells), c * 255 / maxi(1, cells - 1));
            }
            return;
        }
        const int rows = riseCells(settings.mirror);
        for (int x = 0; x < W; x++) {
            const int xp = _host->originX(x);
            const float l = xp < 0 ? 0 : level(xp, W), p = xp < 0 ? 0 : cellOf([&](int k) { return _peak[k]; }, n, xp, W);
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
        const float step = .09f - .078f * settings.speed;
        const int len = m ? riseCells(settings.mirror) : stripCells(), cols = m ? W : 1, n = bands();
        _clock += _dt;
        for (int k = 0; _clock >= step && k < 8; k++) {
            _clock -= step;
            for (int a = len - 1; a > 0; a--)
                for (int c = 0; c < cols; c++) { _v[a * cols + c] = _v[(a - 1) * cols + c]; _h[a * cols + c] = _h[(a - 1) * cols + c]; }
            if (m) {
                float lv[MAX_BANDS];
                for (int i = 0; i < n; i++) lv[i] = level01(autolevel(i, _host->band(i), step));
                for (int c = 0; c < cols; c++) {
                    const float v = cellOf([&](int i) { return lv[i]; }, n, c, cols);
                    _v[c] = u8((int) (v * 255)); _h[c] = u8((int) (v * 255));
                }
            } else {
                _v[0] = u8((int) (level01(autolevel(0, _host->lineSource(), step)) * 255));
                _h[0] = u8((int) (_tone * 255));
            }
        }
        if (!m) {
            for (int x = 0; x < W; x++) { const int c = stripCell(x); stripPut(x, c < 0 ? 0 : _v[c] / 255.f, c < 0 ? 0 : _h[c]); }
            return;
        }
        for (int x = 0; x < W; x++) {
            const int xp = _host->originX(x);
            for (int y = 0; y < H; y++) {
                if (xp < 0) { put(x, y, 0, 0); continue; }
                // Newest row at the top (or in the middle when mirrored), older rows move away.
                const int age = settings.mirror ? riseCell(y, true) : H - 1 - y, i = age * cols + xp;
                put(x, y, _v[i] / 255.f, _h[i]);
            }
        }
    }
    // Pulses: onsets launch comets along a strip, rings from the centre on a matrix.
    void pulses() {
        using namespace detail;
        const bool m = matrix();
        const float line = _host->lineSource();
        float strength[3] = {0, 0, 0};
        const int mask = onsets(strength);
        for (int g = 0; g < 3; g++)
            if (mask & (1 << g)) {
                Comet *slot = &_comets[0];
                for (Comet &c: _comets) { if (!c.on) { slot = &c; break; } if (c.energy < slot->energy) slot = &c; }
                *slot = {0.f, clampf(strength[g] * (.55f + .45f * level01(line)), 0.f, 1.f), uint8_t(g * 85 + 20), true};
            }
        const int n = m ? W * H : stripCells();
        fade(n, release() * .3f);
        const float rmax = m ? maxf(1.f, sqrtf_(float(W * W + H * H)) / 2.f) : 1.f;
        const float cx = m && radial() ? W / 2.f : (W - 1) / 2.f, cy = m && radial() ? _host->radialRow() : (H - 1) / 2.f;
        for (Comet &c: _comets) {
            if (!c.on) continue;
            const float from = c.pos;
            c.pos += _dt * (.25f + 1.75f * settings.speed) * (1.f + .25f * (c.hue / 85));
            if (!m) {
                const int a = (int) (from * (n - 1)), b = mini(n - 1, (int) (c.pos * (n - 1)));
                for (int k = a; k <= b; k++) stamp(k, c.energy, c.hue);
            } else {
                // Ring of radius pos (1 = the farthest corner), about 1.5 LEDs thick.
                const float r0 = from * rmax, r1 = c.pos * rmax, f = 1.f - .6f * c.pos;
                const float lo = maxf(0.f, r0 - 1.5f), hi = r1 + 1.5f;
                for (int y = 0; y < H; y++)
                    for (int x = 0; x < W; x++) {
                        const float dx = x - cx, dy = y - cy, d2 = dx * dx + dy * dy;
                        if (d2 < lo * lo || d2 > hi * hi) continue;
                        const float r = sqrtf_(d2), d = r < r0 ? r0 - r : (r > r1 ? r - r1 : 0.f);
                        stamp(y * W + x, c.energy * f * maxf(0.f, 1.f - d / 1.5f), c.hue);
                    }
            }
            if (c.pos >= 1.f) c.on = false;
        }
        const float glow = .12f * level01(line);
        if (!m) {
            for (int x = 0; x < W; x++) {
                const int c = stripCell(x);
                if (c < 0) { stripPut(x, 0, 0); continue; }
                const float v = _v[c] / 255.f;
                stripPut(x, maxf(v, glow), v > glow ? _h[c] : int(_tone * 255));
            }
            return;
        }
        for (int y = 0; y < H; y++)
            for (int x = 0; x < W; x++) {
                const float v = _v[y * W + x] / 255.f;
                put(x, y, maxf(v, glow), v > glow ? _h[y * W + x] : int(_tone * 255));
            }
    }
    // Sparkles: treble and onsets sprinkle sparks, bass is a soft glow.
    void sparkles() {
        using namespace detail;
        const int n = W * H;
        fade(n, release() * .6f);
        float strength[3] = {0, 0, 0};
        const int mask = onsets(strength);
        _spawn += _dt * n * (.02f + 1.2f * level01(_host->level(2)));
        for (int g = 0; g < 3; g++) if (mask & (1 << g)) _spawn += n * .05f * strength[g];
        for (int k = 0; k < n && _spawn >= 1.f; k++, _spawn -= 1.f) {
            const int g = mask & 4 ? 2 : (mask & 2 ? 1 : (mask & 1 ? 0 : 2));
            stamp(random(n), .75f + random8() / 1020.f, g * 85 + random8() / 8);
        }
        _spawn = minf(_spawn, float(n));
        const float glow = .35f * level01(_host->level(0));
        for (int y = 0; y < H; y++)
            for (int x = 0; x < W; x++) {
                const float v = _v[y * W + x] / 255.f;
                put(x, y, maxf(v, glow), v > glow ? _h[y * W + x] : int(_tone * 120));
            }
    }
    // Mood: the whole lamp breathes with the level, colour follows the tone.
    void mood() {
        using namespace detail;
        _mood = approach(_mood, level01(_host->control()), _dt, .05f, release());
        const float v = clampf(_mood + .35f * _host->pulse(0), 0.f, 1.f);
        const int hue = int(_tone * 200), cells = stripCells();
        for (int y = 0; y < H; y++)
            for (int x = 0; x < W; x++) {
                const int sc = matrix() ? 0 : stripCell(x);
                const int c = matrix() ? (x + y) * 48 / maxi(1, W + H - 2) : (sc < 0 ? 0 : sc * 48 / maxi(1, cells - 1));
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
                const float v = c < 0 ? 0 : level01(_host->wave(c, cells));
                stripPut(x, v, int(v * 255));
            }
            return;
        }
        const int rows = riseCells(settings.mirror);
        for (int x = 0; x < W; x++) {
            const int xp = _host->originX(x);
            const float a = xp < 0 ? 0 : level01(_host->wave(xp, W));
            for (int y = 0; y < H; y++) {
                const int r = riseCell(y, settings.mirror);
                put(x, y, xp < 0 ? 0 : clampf(a * rows - r, 0.f, 1.f), r * 255 / maxi(1, rows - 1));
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
        fade(n, release() * .45f);
        const float level = overall(_host->lineSource());
        for (int k = 0; k < dots; k++) {
            const float b = level01(autolevel(k, bandCell(k, dots), _dt));
            _phase[k] += _dt * (.18f + .045f * k) * (.4f + 1.6f * settings.speed) * (.6f + .8f * level);
            if (b < .05f) continue;
            const int hue = k * 255 / dots;
            if (!m) {
                // Swing around the strip; wider swings for louder bands.
                const float pos = (.5f + .5f * sinTurns(_phase[k])) * (n - 1), half = .6f + 2.f * b;
                for (int i = maxi(0, (int) (pos - half)); i <= mini(n - 1, (int) (pos + half) + 1); i++)
                    stamp(i, b * clampf(half - absf(i - pos) + .5f, 0.f, 1.f), hue);
            } else {
                // Lissajous paths, the dot grows with its band.
                const float px = (.5f + .5f * sinTurns(_phase[k])) * (W - 1);
                const float py = (.5f + .5f * sinTurns(_phase[k] * 1.37f + k * .11f)) * (H - 1);
                const float rad = .7f + b * maxf(1.f, minf(W, H) / 10.f);
                for (int y = maxi(0, (int) (py - rad - 1)); y <= mini(H - 1, (int) (py + rad + 1)); y++)
                    for (int x = maxi(0, (int) (px - rad - 1)); x <= mini(W - 1, (int) (px + rad + 1)); x++) {
                        const float dx = x - px, dy = y - py;
                        stamp(y * W + x, b * clampf(rad - sqrtf_(dx * dx + dy * dy) + .5f, 0.f, 1.f), hue);
                    }
            }
        }
        if (!m) {
            for (int x = 0; x < W; x++) { const int c = stripCell(x); stripPut(x, c < 0 ? 0 : _v[c] / 255.f, c < 0 ? 0 : _h[c]); }
            return;
        }
        for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) { const int i = y * W + x; put(x, y, _v[i] / 255.f, _h[i]); }
    }

#if VIZ_WITH_HEAVY
    // Fire: bass and onsets feed the flames; strip burns from the start (or
    // centre), matrix from the bottom with a flame per band.
    void fire() {
        using namespace detail;
        const bool m = matrix();
        const float step = .033f - .022f * settings.speed, level = overall(_host->lineSource());
        const int cooling = 90 - (int) (settings.decay * 60.f), n = bands();
        float strength[3] = {0, 0, 0};
        const int mask = onsets(strength);
        _clock += _dt;
        const int rows = m ? riseCells(settings.mirror) : stripCells(), cols = m ? W : 1;
        for (int s = 0; _clock >= step && s < 6; s++) {
            _clock -= step;
            float lv[MAX_BANDS];
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
                const float feed = m ? cellOf([&](int i) { return lv[i]; }, n, c, cols) : level;
                const float boost = (mask & 1 ? strength[0] : 0) + (mask & 2 ? .5f * strength[1] : 0);
                if (feed > .03f && random8() < (int) (20 + 220 * feed)) {
                    const int at = random(mini(rows, m ? 2 : maxi(2, rows / 10)));
                    _v[at * cols + c] = u8(_v[at * cols + c] + (int) (((m ? 80 : 120) + random8() / 2) * (.4f + .6f * feed)));
                }
                if (boost > 0)
                    for (int r = 0; r < mini(rows, m ? 2 : 3); r++)
                        _v[r * cols + c] = u8(_v[r * cols + c] + (int) ((m ? 90 : 140) * boost));
            }
        }
        auto flame = [&](int x, int y, int i) { const int heat = _v[i]; put(x, y, minf(1.f, heat / 170.f), heat * 215 / 255); };
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
        _mood = approach(_mood, level01(_host->lineSource()), _dt, .08f, release());
        _phase[0] += _dt * (.25f + 2.5f * _mood) * (.3f + 1.4f * settings.speed);
        const float z = _phase[0] * 256.f, threshold = .5f - .3f * _mood, flash = .3f * _host->pulse(0);
        const float gate = minf(1.f, _mood * 10.f);
        const float factor = 256.f * (m ? 3.f / maxi(W, H) : 5.f / maxi(1, stripCells()));
        for (int x = 0; x < W; x++) {
            const int c = m ? _host->originX(x) : stripCell(x);
            for (int y = 0; y < H; y++) {
                if (c < 0) { put(x, y, 0, 0); continue; }
                // Strips use 2-D noise (position, time): a fixed y = 0 plane of
                // 3-D noise sits on the lattice and barely varies.
                const uint16_t raw = m ? _host->noise(uint32_t(c * factor * 256), uint32_t((y * factor * 1.6f + 77.f) * 256), uint32_t(z * 256))
                                       : _host->noise(uint32_t(c * factor * 256), uint32_t(z * 256));
                const float nz = (raw / 16) / 4095.f;
                const float v = (clampf((nz - threshold) * 3.f, 0.f, 1.f) * (.45f + .55f * _mood) + (nz > threshold ? flash : 0.f)) * gate;
                put(x, y, v, int(_tone * 150 + nz * 90));
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
        const float level = overall(_host->lineSource());
        _phase[0] += _dt * (.02f + .25f * level) * (.3f + 1.4f * settings.speed);
        // Band position t: 0 = bass .. 1 = treble, mirrored so there is no seam.
        auto bandAt = [&](float turns, float &t) { t = absf(2.f * fract(turns) - 1.f); return _level[mini(count - 1, (int) (t * count))]; };
        if (!m) {
            for (int x = 0; x < W; x++) {
                const int xp = _host->originX(x);
                if (xp < 0) { stripPut(x, 0, 0); continue; }
                float t;
                const float l = bandAt(float(xp) / W + _phase[0], t);
                stripPut(x, l, int(t * 255));
            }
            return;
        }
        const float cx = W / 2.f - .5f, cy = radial() ? _host->radialRow() : H / 2.f - .5f, rmax = maxf(1.f, minf(W, H) / 2.f);
        for (int x = 0; x < W; x++) {
            const int xp = _host->originX(x);
            for (int y = 0; y < H; y++) {
                if (xp < 0) { put(x, y, 0, 0); continue; }
                const float dx = xp - cx, dy = y - cy, r = sqrtf_(dx * dx + dy * dy) / rmax;
                float t;
                const float l = bandAt(angleTurns(dy, dx) + _phase[0], t);
                // Ray length follows the band; a bass-driven core glows in the middle.
                const float ray = clampf((l * 1.05f - r) * rmax + .5f, 0.f, 1.f);
                const float core = clampf((_level[0] * .35f - r) * rmax * .5f + .5f, 0.f, 1.f) * _level[0];
                put(x, y, maxf(ray * (.45f + .55f * l), core), ray >= core ? int(t * 255) : 0);
            }
        }
    }
    // Blobs: a lava lamp of four blobs (bass, bass, mid, treble) that swell
    // with their band and merge.
    void blobs() {
        using namespace detail;
        const bool m = matrix();
        _mood = approach(_mood, overall(_host->lineSource()), _dt, .05f, release());
        static constexpr int groups[4] = {0, 0, 1, 2};
        float bx[4], by[4], br[4], group[3];
        int bh[4];
        for (int g = 0; g < 3; g++) group[g] = level01(autolevel(g, _host->level(g), _dt));
        const float span = m ? minf(W, H) : stripCells();
        for (int k = 0; k < 4; k++) {
            const int g = groups[k];
            _phase[k] += _dt * (.05f + .023f * k) * (.3f + 1.7f * settings.speed) * (.5f + _mood);
            _phase[4 + k] = approach(_phase[4 + k], group[g] + .6f * _host->pulse(g), _dt, .03f, release());
            const float grow = _phase[4 + k];
            br[k] = span * (m ? .05f + .16f * grow : .02f + .08f * grow);
            bx[k] = (.5f + .45f * sinTurns(_phase[k] + k * .25f)) * ((m ? W : stripCells()) - 1);
            by[k] = (.5f + .45f * sinTurns(_phase[k] * 1.31f + k * .4f)) * (H - 1);
            bh[k] = g * 85 + 20;
        }
        const float gate = minf(1.f, _mood * 4.f);
        for (int x = 0; x < W; x++) {
            const int c = m ? _host->originX(x) : stripCell(x);
            for (int y = 0; y < H; y++) {
                if (c < 0) { put(x, y, 0, 0); continue; }
                float field = 0, hue = 0;
                for (int k = 0; k < 4; k++) {
                    const float dx = c - bx[k], dy = m ? y - by[k] : 0.f, f = br[k] * br[k] / (dx * dx + dy * dy + .3f);
                    field += f; hue += f * bh[k];
                }
                put(x, y, clampf((field - .7f) * 1.6f, 0.f, 1.f) * gate, int(hue / maxf(1e-3f, field)));
            }
        }
    }
#endif
};
} // namespace Viz
