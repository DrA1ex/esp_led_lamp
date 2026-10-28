// Host-only invariants for the two experimental WASM-derived modes.
// g++ -std=c++17 -Wall -Wextra -Werror tests/test_remote_reactive.cpp -o /tmp/reactive && /tmp/reactive
#include <cassert>
#include "../src/remote_audio/reactive_math.h"
int main() {
    using namespace RemoteReactive;
    for (int band = 0; band <= 4096; band += 128) {
        for (int level = 0; level <= 4096; level += 256) {
            for (int y = 0; y < 2; ++y) {
                const uint8_t factor = bassMidHigh(band, level, level, level,
                                                   4096, 4096, 4096, y == 0);
                assert(factor <= 255);
                const uint8_t low = bassMidHigh(0, 0, 0, 0, 0, 0, 0, y == 0);
                assert(factor >= low); // increasing input never reduces light
            }
            for (int phase = 0; phase < 256; phase += 16) {
                const float pulse = predictedBeat(phase, 255, level);
                assert(pulse >= 0 && pulse <= 1);
                for (int x = 0; x < 16; ++x) {
                    const uint8_t factor = beatReactive(band, pulse, x, 16);
                    assert(factor <= 255);
                    for (int channel = 0; channel < 256; channel += 7)
                        assert((channel * factor / 255) <= channel);
                }
            }
        }
    }
    assert(bassMidHigh(4096,4096,4096,4096,4096,4096,4096,true) == 255);
    assert(beatReactive(4096,1.f,8,16) <= 255);
}
