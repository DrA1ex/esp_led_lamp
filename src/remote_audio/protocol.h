#pragma once

// Transport-independent Remote Audio Protocol v1. No Arduino dependencies.
#include <stddef.h>
#include <stdint.h>

namespace RemoteProtocol {
constexpr size_t FRAME_SIZE = 102;
constexpr uint8_t VERSION = 1;
constexpr uint8_t BAND_COUNT = 32;
constexpr uint16_t MAX_LEVEL = 4096;

struct Frame {
    uint8_t flags = 0;
    uint32_t session = 0;
    uint32_t sequence = 0;
    uint32_t presentation_ms = 0;
    uint16_t rms = 0, bass = 0, mid = 0, high = 0, peak = 0, onset = 0;
    uint8_t onset_mask = 0, beat_phase = 0;
    uint16_t bpm_x10 = 0;
    uint8_t beat_confidence = 0;
    uint16_t bands[BAND_COUNT] = {};
};

inline uint16_t read16(const uint8_t *p) {
    return uint16_t(p[0]) | uint16_t(p[1]) << 8;
}
inline uint32_t read32(const uint8_t *p) {
    return uint32_t(read16(p)) | uint32_t(read16(p + 2)) << 16;
}
inline bool validLevel(uint16_t n) { return n <= MAX_LEVEL; }

// Explicit byte decoding; never cast the incoming bytes to a packed struct.
inline bool decode(const uint8_t *data, size_t len, Frame &out) {
    if (!data || len != FRAME_SIZE ||
        data[0] != 'E' || data[1] != 'L' || data[2] != 'A' || data[3] != 'F' ||
        data[4] != VERSION || data[5] != 1 ||
        (data[6] & ~uint8_t(1)) != 0 || data[7] != BAND_COUNT ||
        (data[32] & ~uint8_t(7)) != 0 || data[37] != 0) return false;

    Frame parsed;
    parsed.flags = data[6];
    parsed.session = read32(data + 8);
    parsed.sequence = read32(data + 12);
    parsed.presentation_ms = read32(data + 16);
    parsed.rms = read16(data + 20);
    parsed.bass = read16(data + 22);
    parsed.mid = read16(data + 24);
    parsed.high = read16(data + 26);
    parsed.peak = read16(data + 28);
    parsed.onset = read16(data + 30);
    parsed.onset_mask = data[32];
    parsed.beat_phase = data[33];
    parsed.bpm_x10 = read16(data + 34);
    parsed.beat_confidence = data[36];
    if (!validLevel(parsed.rms) || !validLevel(parsed.bass) ||
        !validLevel(parsed.mid) || !validLevel(parsed.high) ||
        !validLevel(parsed.peak) || !validLevel(parsed.onset) ||
        parsed.session == 0) return false;
    for (uint8_t i = 0; i < BAND_COUNT; ++i) {
        parsed.bands[i] = read16(data + 38 + 2 * i);
        if (!validLevel(parsed.bands[i])) return false;
    }
    out = parsed;
    return true;
}
} // namespace RemoteProtocol
