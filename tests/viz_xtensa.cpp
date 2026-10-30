// Instantiates the visualization engine for the ESP8266 against an opaque
// host so nothing folds away; tests/check_viz_integer.sh compiles it.
#include "../src/remote_audio/viz_engine.h"
extern "C" int ext_i(int); extern "C" uint8_t ext_r(); extern "C" int16_t ext_s(uint16_t);
extern "C" uint16_t ext_n(uint32_t, uint32_t, uint32_t); extern "C" void ext_put(int, int, uint8_t, uint8_t);
extern "C" void ext_rgb(int, int, uint8_t, uint8_t, uint8_t);
struct H {
    int width() const { return ext_i(0); } int height() const { return ext_i(1); }
    int originX(int x) const { return ext_i(x + 2); } bool radial() const { return ext_i(3); }
    int radialRow() const { return ext_i(4); } int transfer(int v) const { return ext_i(v); }
    int control() const { return ext_i(5); } int lineSource() const { return ext_i(6); }
    int level(int g) const { return ext_i(7 + g); } int pulse(int g) const { return ext_i(10 + g); }
    int beat() const { return ext_i(78); }
    int bandCount() const { return ext_i(13); } int band(int i) const { return ext_i(14 + i); }
    int wave(int i, int s) const { return ext_i(i * s); }
    uint8_t random8() { return ext_r(); } int16_t sin16(uint16_t t) const { return ext_s(t); }
    uint16_t noise(uint32_t x, uint32_t y) const { return ext_n(x, y, 0); }
    uint16_t noise(uint32_t x, uint32_t y, uint32_t z) const { return ext_n(x, y, z); }
    void put(int x, int y, uint8_t v, uint8_t h) { ext_put(x, y, v, h); }
    void rgb(int x, int y, uint8_t r, uint8_t g, uint8_t b) { ext_rgb(x, y, r, g, b); }
};
Viz::Engine<H, 256> engine;
H host;
void viz_frame(uint8_t mode, uint16_t dt) { engine.render(host, mode, dt); }
