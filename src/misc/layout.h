#pragma once
#include <stdint.h>

// How the LEDs are mounted, saved on the lamp ("layout" storage) and set from
// the lab's Layout tab (docs/REMOTE_AUDIO_PROTOCOL.md, "Layout"). Effects
// draw in logical columns and rows; Led::get_index maps them to the wiring:
// flip_x / flip_y mirror the columns / rows, and on a ring (a closed strip,
// a matrix wrapped round a cylinder) the picture turns `offset` LEDs along
// the wiring, so the logical centre WIDTH / 2 faces the viewer. An open strip
// cannot turn: there the offset moves the origin of the audio modes
// (RemoteReactive::OriginMap) and LEDs past the end stay dark.
struct __attribute__((packed)) LayoutConfig {
    bool flip_x = false;
    bool flip_y = false;
    bool ring = false;
    int16_t offset = 0; // LEDs along the wiring, -WIDTH..WIDTH

    bool operator==(const LayoutConfig &o) const {
        return flip_x == o.flip_x && flip_y == o.flip_y && ring == o.ring && offset == o.offset;
    }
};

namespace Layout {
// Columns the ring turns by, 0..width - 1 (0 on an open strip).
inline int rotation(const LayoutConfig &l, int width) {
    if (!l.ring || width <= 0) return 0;
    const int r = l.offset % width;
    return r < 0 ? r + width : r;
}
// Logical columns the audio origin moves on an open strip (0 on a ring): the
// offset counts along the wiring, which runs backwards when flipped.
inline int originShift(const LayoutConfig &l) { return l.ring ? 0 : l.flip_x ? -l.offset : l.offset; }
// Led::get_index: the row-serpentine wiring index of logical (x, y).
inline int index(int x, int y, int width, int height, bool flip_x, bool flip_y, int rotate) {
    if (flip_x) x = width - 1 - x;
    if (flip_y) y = height - 1 - y;
    if (rotate && (x += rotate) >= width) x -= width;
    return (y & 1 ? width - 1 - x : x) + y * width;
}

// Orientation test, the same picture as the lab preview's (render.cpp
// render_test_pattern): red (0, 0), green (W - 1, 0), blue (0, H - 1),
// white (W - 1, H - 1), a yellow centre column where the Center modes
// centre, and dim edges on a matrix. put(x, y, 0xRRGGBB) in logical cells.
template<typename Put>
void testPattern(int width, int height, int shift, Put put) {
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) put(x, y, 0x000000);
    if (height > 2) {
        for (int x = 0; x < width; ++x) { put(x, 0, 0x300000); put(x, height - 1, 0x000030); }
        for (int y = 0; y < height; ++y) { put(0, y, 0x301800); put(width - 1, y, 0x003018); }
    }
    const int centre = width / 2 + shift;
    if (centre >= 0 && centre < width)
        for (int y = 0; y < height; ++y) put(centre, y, 0xffc000);
    put(0, 0, 0xff0000);
    put(width - 1, 0, 0x00ff00);
    if (height > 1) {
        put(0, height - 1, 0x0000ff);
        put(width - 1, height - 1, 0xffffff);
    }
}
}
