// Host-only invariants for the WASM-derived reactive math shared with the lab.
// g++ -std=c++17 -Wall -Wextra -Werror tests/test_remote_reactive.cpp -o /tmp/reactive && /tmp/reactive
#include <cassert>
#include <initializer_list>
#include "../src/remote_audio/reactive_math.h"
#include "../src/audio/transfer.h"
int main() {
    using namespace RemoteReactive;
    const Half halves[] = {Half::UPPER, Half::LOWER, Half::BOTH};
    for (int band = 0; band <= 4096; band += 128) {
        for (int level = 0; level <= 4096; level += 256) {
            for (const Half half : halves) {
                const uint8_t factor = bassMidHigh(band, level, level, level,
                                                   4096, 4096, 4096, half);
                const uint8_t low = bassMidHigh(0, 0, 0, 0, 0, 0, 0, half);
                assert(factor >= low); // increasing input never reduces light
            }
            for (int phase = 0; phase < 256; phase += 16) {
                const float pulse = predictedBeat(phase, 255, level, 1200);
                assert(pulse >= 0 && pulse <= 1);
                for (int x = 0; x < 16; ++x) {
                    const uint8_t factor = beatReactive(band, pulse, x, 16);
                    for (int channel = 0; channel < 256; channel += 7)
                        assert((channel * factor / 255) <= channel);
                }
            }
        }
    }
    assert(bassMidHigh(4096,4096,4096,4096,4096,4096,4096,Half::UPPER) == 255);
    assert(beatReactive(4096,1.f,8,16) <= 255);

    // Rows: a single-row strip blends bass and mid; odd heights split at H/2 like the lab.
    assert(halfForRow(0, 1) == Half::BOTH);
    assert(halfForRow(0, 3) == Half::UPPER && halfForRow(1, 3) == Half::LOWER);
    assert(halfForRow(7, 16) == Half::UPPER && halfForRow(8, 16) == Half::LOWER);
    const uint8_t bassOnly = bassMidHigh(0, 4096, 0, 0, 0, 0, 0, Half::BOTH);
    assert(bassOnly > bassMidHigh(0, 0, 0, 0, 0, 0, 0, Half::BOTH));
    assert(bassOnly < bassMidHigh(0, 4096, 0, 0, 0, 0, 0, Half::UPPER));

    // Beat pulse: 1 at the beat, gone after 230 ms at any tempo.
    assert(beatPulse(0, 1200) == 1.f);
    assert(beatPulse(128, 1200) == 0.f); // half a beat at 120 BPM = 250 ms
    assert(beatPulse(20, 1200) > 0.f && beatPulse(20, 1200) < 1.f);

    // Shaped spectrum: zero tuning is exactly the stock dim8_raw curve.
    for (int v = 0; v < 256; ++v) {
        assert(shapedLevel(v, 0.f, 0.f) == dim8Raw(v));
        assert(shapedLevel(v, 1.f, 0.f) >= dim8Raw(v));
        if (v) assert(shapedLevel(v, .7f, 0.f) >= shapedLevel(v - 1, .7f, 0.f));
    }
    assert(shapedLevel(0, 0.f, .5f) >= 127);
    assert(onsetGroup(0, 16) == 0 && onsetGroup(6, 16) == 1 && onsetGroup(15, 16) == 2);

    // Control sources read the documented frame fields.
    RemoteProtocol::Frame f{};
    f.rms = 100; f.bass = 200; f.mid = 300; f.high = 400; f.peak = 500;
    f.beat_phase = 0; f.bpm_x10 = 1200; f.beat_confidence = 255;
    for (auto &b : f.bands) b = 1000;
    const uint16_t pulses[3] = {10, 700, 20};
    assert(controlValue(f, pulses, ControlSource::LOUDNESS) == 100);
    assert(controlValue(f, pulses, ControlSource::BASS) == 200);
    assert(controlValue(f, pulses, ControlSource::MID) == 300);
    assert(controlValue(f, pulses, ControlSource::TREBLE) == 400);
    assert(controlValue(f, pulses, ControlSource::PEAK) == 500);
    assert(controlValue(f, pulses, ControlSource::ONSET) == 700);
    assert(controlValue(f, pulses, ControlSource::SPECTRAL) == 1550);
    assert(controlValue(f, pulses, ControlSource::BEAT) == 4096);
    for (auto &b : f.bands) b = 4096;
    assert(controlValue(f, pulses, ControlSource::SPECTRAL) == 4096);
    // Signal window: the default is the stock formula; the window clamps/gates.
    for (uint32_t v = 0; v <= 4096; v += 64)
        assert(audioTransfer(v, 4096, 255, 40, 200) == int32_t(40 * 255 / 255 + v * (200 - 40) / 4096));
    AudioWindow window;
    window.in_min = 50; window.in_max = 100;
    assert(audioTransfer(0, 255, 255, 150, 250, window) == 150);
    assert(audioTransfer(49, 255, 255, 150, 250, window) == 150);
    assert(audioTransfer(100, 255, 255, 150, 250, window) == 250);
    assert(audioTransfer(255, 255, 255, 150, 250, window) == 250);
    assert(audioTransfer(75, 255, 255, 150, 250, window) == 200);
    window.gate = true;
    assert(audioTransfer(49, 255, 255, 150, 250, window) == 0);
    assert(audioTransfer(50, 255, 255, 150, 250, window) == 150);

    // Origin: the default reproduces the stock column / centre geometry.
    for (int width : {1, 7, 16, 153}) {
        const OriginMap stock(width, 1, Origin{});
        for (int x = 0; x < width; ++x) {
            assert(stock.x(x) == x);
            const int center = width / 2;
            assert(stock.centeredIndex(x, 0) == (x <= center ? center - x : x - center));
            assert(stock.beatRadius(x, 0) == absf(float(x) - (float(width) - 1.f) / 2.f) / (float(width) / 2.f));
            // AudioEffectManager::line_centered blanks i < inactive and W-1-i < inactive.
            for (int reach = 0; reach <= width / 2; ++reach) {
                const int inactive = width / 2 - reach;
                const bool lit = !(x < inactive || width - 1 - x < inactive);
                assert(stock.lineCenteredLit(x, 0, reach) == lit);
            }
        }
    }
    // Shift: without ring LEDs before the start stay dark, with ring they wrap.
    Origin shifted;
    shifted.shift = 250;
    const OriginMap open_strip(100, 1, shifted);
    assert(open_strip.x(24) == -1 && open_strip.x(25) == 0 && open_strip.x(99) == 74);
    shifted.ring = true;
    const OriginMap ring(100, 1, shifted);
    assert(ring.x(25) == 0 && ring.x(24) == 99 && ring.x(0) == 75);
    shifted.shift = -250;
    assert(OriginMap(100, 1, shifted).x(0) == 25 && OriginMap(100, 1, shifted).x(75) == 0);
    // Reverse: the effect starts at the last LED.
    Origin reversed;
    reversed.reverse = true;
    assert(OriginMap(16, 1, reversed).x(15) == 0 && OriginMap(16, 1, reversed).x(0) == 15);
    // Radial centre on a matrix: corners get the last provider index, the centre 0.
    Origin radial;
    radial.radial = true;
    const OriginMap disc(16, 16, radial);
    assert(disc.radial() && !OriginMap(16, 1, radial).radial());
    assert(disc.centeredIndex(0, 0) == 8 && disc.centeredIndex(8, 7) <= 1);
    assert(disc.beatRadius(0, 0) <= 1.f && disc.beatRadius(8, 8) < .1f);
    assert(!disc.lineCenteredLit(0, 0, 4) && disc.lineCenteredLit(8, 8, 1) && !disc.lineCenteredLit(8, 8, 0));
}
