// Host-only checks of the remote visualizations (viz_engine.h, shared with the
// lab preview) on every geometry class: silence stays dark, music lights the
// LEDs, strip rows match, mirrored strips are symmetric, band counts are used.
// g++ -std=c++17 -Wall -Wextra -Werror tests/test_remote_viz.cpp -o /tmp/remote_viz && /tmp/remote_viz
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>
#include "../src/remote_audio/viz_engine.h"

struct TestHost {
    int w, h;
    std::vector<float> v;
    std::vector<int> hue;
    float bands[64] = {}, levels[3] = {}, pulses[3] = {}, ctrl = 0, waves[128] = {};
    int count = 32;
    uint32_t seed = 1337;
    TestHost(int width, int height) : w(width), h(height), v(width * height), hue(width * height) {}

    int width() const { return w; }
    int height() const { return h; }
    bool round = false;
    // The test keeps its signals as 0..1 floats; the engine sees Q12 integers.
    static int q(float x) { return int(std::lround(x * 4096)); }
    int originX(int x) const { return x; }
    bool radial() const { return round; }
    int radialRow() const { return (h - 1) * 128; }
    int transfer(int x) const { return x; }
    int control() const { return q(ctrl); }
    int lineSource() const { return q(ctrl); }
    int level(int g) const { return q(levels[g]); }
    int pulse(int g) const { return q(pulses[g]); }
    int bandCount() const { return count; }
    int band(int i) const { return q(bands[i]); }
    int wave(int index, int span) const {
        if (span > 128) index = index * 127 / (span - 1);
        return q(waves[index < 127 ? index : 127]);
    }
    uint8_t random8() { seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5; return uint8_t(seed); }
    int16_t sin16(uint16_t t) const { return int16_t(std::lround(std::sin(t * 6.2831853 / 65536) * 32767)); }
    uint16_t noise(uint32_t x, uint32_t y, uint32_t z = 0) const {
        // Smooth enough for coverage: sums of sines over the 16.16 coordinates.
        const double a = x / 65536.0, b = y / 65536.0, c = z / 65536.0;
        return uint16_t(32768 + 16000 * std::sin(a * 2.1 + c * 1.3) * std::cos(b * 1.7 - c * .9));
    }
    void put(int x, int y, uint8_t value, uint8_t hue_value) {
        assert(x >= 0 && x < w && y >= 0 && y < h);
        v[y * w + x] = value / 255.f; hue[y * w + x] = hue_value;
    }
    int lit() const { int n = 0; for (float x: v) n += x > .1f; return n; }
};

// The lab test's "music": rising spectrum, levels, a bass/mid/treble onset every 6 frames.
static void music(TestHost &t, int k) {
    for (int i = 0; i < 64; i++) t.bands[i] = .3f + .6f * std::fabs(std::sin(i * .4f + k * .3f));
    for (int g = 0; g < 3; g++) t.pulses[g] = 0;
    t.pulses[k % 3] = k % 6 < 3 ? 1.f : 0.f;
    t.ctrl = .8f;
    for (int i = 0; i < 128; i++) t.waves[i] = .4f + .5f * std::fabs(std::sin(i * .2f));
    t.levels[0] = .7f; t.levels[1] = .5f; t.levels[2] = .6f;
}
static void silence(TestHost &t, int) {
    for (float &b: t.bands) b = 0;
    for (int g = 0; g < 3; g++) t.levels[g] = t.pulses[g] = 0;
    t.ctrl = 0;
    for (float &x: t.waves) x = 0;
}

template<int PIX>
static TestHost run(int w, int h, uint8_t mode, void (*fill)(TestHost &, int), int frames = 40,
                    bool mirror = false, int count = 32, bool autolevel = false, bool round = false) {
    static Viz::Engine<TestHost, PIX> engine;
    engine.invalidate();
    engine.settings = Viz::Settings{};
    engine.settings.mirror = mirror;
    engine.settings.autolevel = autolevel;
    TestHost t(w, h);
    t.count = count;
    t.round = round;
    for (int k = 0; k < frames; k++) { fill(t, k); engine.render(t, mode, 17); }
    return t;
}

int main() {
    // Integer helpers: exact square roots, angles within 0.001 turn of atan2.
    for (uint32_t v = 0; v < 2000000; v += 997) {
        const uint32_t r = Viz::detail::isqrt(v);
        assert(r * r <= v && (r + 1) * (r + 1) > v);
    }
    for (int a = 0; a < 360; a += 3) {
        const double rad = a * 3.14159265358979 / 180;
        const int x = int(std::lround(std::cos(rad) * 900)), y = int(std::lround(std::sin(rad) * 900));
        double turns = Viz::detail::angle16(y, x) / 65536.0, want = std::atan2(y, x) / 6.28318530718;
        if (want < 0) want += 1;
        double err = std::fabs(turns - want);
        if (err > .5) err = 1 - err;
        assert(err < .001);
    }
    const int geometries[][2] = {{7, 1}, {153, 1}, {72, 2}, {16, 16}, {46, 1}};
    for (uint8_t mode = 0; mode < Viz::MODE_COUNT; mode++) {
        assert(Viz::available(mode));
        for (const auto &g: geometries) {
            const int w = g[0], h = g[1];
            const TestHost quiet = run<256>(w, h, mode, silence, 40, false, 32, true);
            assert(quiet.lit() <= w * h / 50);
            const TestHost loud = run<256>(w, h, mode, music, 40, false, 32, true);
            if (loud.lit() == 0) { std::printf("mode %u %dx%d dark\n", mode, w, h); return 1; }
            // Every strip row shows the same, except the random Sparkles.
            if (h > 1 && h < 4 && mode != Viz::SPARKLES)
                for (int x = 0; x < w; x++) assert(loud.v[x] == loud.v[w + x]);
        }
    }
    // Mirrored strip VU / spectrum / waveform are symmetric around the centre.
    for (uint8_t mode: {Viz::VU_METER, Viz::SPECTRUM_BARS, Viz::WAVEFORM}) {
        const TestHost t = run<256>(61, 1, mode, music, 40, true);
        for (int x = 0; x < 30; x++) assert(t.v[x] == t.v[60 - x]);
    }
    // 64 bands: only the last band lit puts light at the strip end only.
    {
        const TestHost t = run<256>(64, 1, Viz::SPECTRUM_BARS, [](TestHost &h, int) { h.bands[63] = 1; }, 40, false, 64);
        assert(t.v[63] > .1f && t.v[31] == 0.f);
    }
    // One loud band of 32 fills its matrix column, a silent one stays dark.
    {
        const TestHost t = run<512>(32, 16, Viz::SPECTRUM_BARS, [](TestHost &h, int) { h.bands[31] = 1; });
        assert(t.v[31] > .1f && t.v[15 * 32 + 31] > .1f && t.v[0] == 0.f);
    }
    // VU on a strip fills from the start up to the level.
    {
        const TestHost t = run<256>(100, 1, Viz::VU_METER, [](TestHost &h, int) { h.ctrl = .5f; });
        assert(t.v[10] > .1f && t.v[80] == 0.f);
    }
    // Radial VU on a matrix: lit around the centre, dark in the corners at half level.
    {
        const TestHost t = run<256>(16, 16, Viz::VU_METER, [](TestHost &h, int) { h.ctrl = .5f; }, 40, false, 32, false, true);
        assert(t.v[8 * 16 + 8] > .1f && t.v[0] == 0.f && t.v[255] == 0.f);
    }
    // A geometry above the pixel budget renders nothing instead of overflowing.
    {
        const TestHost t = run<16>(16, 16, Viz::VU_METER, music);
        assert(t.lit() == 0);
    }
    std::printf("PASS remote visualizations: %d modes on strips 7/46/153, 72x2, 16x16, 32x16; silence, rows, mirror, bands\n",
                int(Viz::MODE_COUNT));
    return 0;
}
