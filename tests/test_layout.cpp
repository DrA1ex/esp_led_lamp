// Lamp layout: Led::get_index mapping, the open-strip origin and the
// orientation test pattern.
// g++ -std=c++17 -Wall -Wextra -Werror tests/test_layout.cpp -o /tmp/layout && /tmp/layout
#include <cassert>
#include <cstdio>
#include <vector>
#include "../src/misc/layout.h"
#include "../src/remote_audio/reactive_math.h"

static int index(const LayoutConfig &l, int x, int y, int w, int h) {
    return Layout::index(x, y, w, h, l.flip_x, l.flip_y, Layout::rotation(l, w));
}

int main() {
    // The default layout is the stock row-serpentine Led::get_index.
    const LayoutConfig stock;
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 6; ++x) assert(index(stock, x, y, 6, 4) == (y & 1 ? 5 - x : x) + y * 6);

    // Every layout maps the logical cells one-to-one onto the wiring.
    for (int w: {1, 7, 16, 153}) for (int h: {1, 2, 16}) for (int f = 0; f < 8; ++f)
        for (int offset: {0, 1, -1, 5, -w, w}) {
            LayoutConfig l;
            l.flip_x = f & 1; l.flip_y = f & 2; l.ring = f & 4; l.offset = int16_t(offset);
            std::vector<int> seen(size_t(w * h), 0);
            for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
                const int i = index(l, x, y, w, h);
                assert(i >= 0 && i < w * h && !seen[size_t(i)]++);
            }
        }

    // Flips mirror columns / rows (a 2-row strip: row 1 runs backwards).
    LayoutConfig flip;
    flip.flip_x = true;
    assert(index(flip, 0, 0, 72, 2) == 71 && index(flip, 71, 0, 72, 2) == 0 && index(flip, 0, 1, 72, 2) == 72);
    flip.flip_x = false; flip.flip_y = true;
    assert(index(flip, 0, 0, 16, 16) == 15 * 16 + 15 && index(flip, 0, 15, 16, 16) == 0);

    // A ring turns along the wiring: logical centre WIDTH / 2 lands offset LEDs further.
    LayoutConfig ring;
    ring.ring = true; ring.offset = 20;
    assert(Layout::rotation(ring, 153) == 20 && index(ring, 76, 0, 153, 1) == 96);
    assert(index(ring, 140, 0, 153, 1) == 7); // wraps past the end
    ring.offset = -20;
    assert(Layout::rotation(ring, 153) == 133 && index(ring, 76, 0, 153, 1) == 56);
    ring.flip_x = true; ring.offset = 3;
    assert(index(ring, 0, 0, 16, 1) == 2); // flipped first, then turned
    assert(Layout::originShift(ring) == 0);

    // An open strip cannot turn: the offset moves the audio origin instead,
    // backwards in logical columns when the strip is flipped.
    LayoutConfig open;
    open.offset = 10;
    assert(Layout::rotation(open, 100) == 0 && Layout::originShift(open) == 10);
    open.flip_x = true;
    assert(Layout::originShift(open) == -10);
    const RemoteReactive::OriginMap moved(100, 1, RemoteReactive::Origin{}, 10);
    assert(moved.x(10) == 0 && moved.x(9) == -1 && moved.x(99) == 89);

    // Orientation test: corners and the centre column.
    std::vector<uint32_t> px(16 * 16);
    Layout::testPattern(16, 16, 0, [&](int x, int y, uint32_t c) { px[size_t(y * 16 + x)] = c; });
    assert(px[0] == 0xff0000 && px[15] == 0x00ff00 && px[15 * 16] == 0x0000ff && px[255] == 0xffffff);
    assert(px[5 * 16 + 8] == 0xffc000 && px[5 * 16 + 7] == 0);
    std::vector<uint32_t> strip(46);
    Layout::testPattern(46, 1, -3, [&](int x, int, uint32_t c) { strip[size_t(x)] = c; });
    assert(strip[0] == 0xff0000 && strip[45] == 0x00ff00 && strip[20] == 0xffc000 && strip[23] == 0);
    std::vector<uint32_t> off(8, 1);
    Layout::testPattern(8, 1, 40, [&](int x, int, uint32_t c) { off[size_t(x)] = c; });
    assert(off[4] == 0); // a centre past the end is not drawn
    std::puts("layout: ok");
}
