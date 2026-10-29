// Native, dependency-free protocol regression tests:
// g++ -std=c++17 -Wall -Wextra -Werror tests/test_remote_audio_protocol.cpp -o /tmp/remote_audio_protocol_test && /tmp/remote_audio_protocol_test
#include <cassert>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include "../src/remote_audio/protocol.h"

static void put16(uint8_t *p, uint16_t n) { p[0] = uint8_t(n); p[1] = uint8_t(n >> 8); }
static void put32(uint8_t *p, uint32_t n) { put16(p, uint16_t(n)); put16(p + 2, uint16_t(n >> 16)); }

int main() {
    using namespace RemoteProtocol;
    uint8_t packet[MAX_FRAME_SIZE] = {};
    static_assert(MAX_FRAME_SIZE == 166, "38-byte header + 64 bands");
    std::memcpy(packet, "ELAF", 4);
    packet[4] = 1; packet[5] = 1; packet[7] = 32;
    put32(packet + 8, 0x01234567);
    put32(packet + 12, 0xFFFFFFFF);
    put32(packet + 16, 0xFFFFFFF0);
    put16(packet + 20, 4096);
    put16(packet + 38, 3000);
    put16(packet + 100, 4096);
    const size_t size = frameSize(32); // 102
    Frame frame;
    assert(decode(packet, size, frame));
    assert(frame.session == 0x01234567 && frame.sequence == 0xFFFFFFFF);
    assert(frame.presentation_ms == 0xFFFFFFF0);
    assert(frame.rms == 4096 && frame.band_count == 32 && frame.bands[0] == 3000 && frame.bands[31] == 4096);
    assert(!decode(packet, size - 1, frame));
    packet[0] = 'X'; assert(!decode(packet, size, frame)); packet[0] = 'E';
    packet[4] = 2; assert(!decode(packet, size, frame)); packet[4] = 1;
    packet[6] = 2; assert(!decode(packet, size, frame)); packet[6] = 1;
    packet[7] = 31; assert(!decode(packet, size, frame)); packet[7] = 32;
    packet[32] = 8; assert(!decode(packet, size, frame)); packet[32] = 0;
    packet[37] = 1; assert(!decode(packet, size, frame)); packet[37] = 0;
    put16(packet + 20, 4097); assert(!decode(packet, size, frame));
    put16(packet + 20, 4096);
    put16(packet + 100, 4097); assert(!decode(packet, size, frame));
    put16(packet + 100, 4096);
    put32(packet + 8, 0); assert(!decode(packet, size, frame));
    put32(packet + 8, 0x01234567);
    assert(decode(packet, size, frame));

    // 16 and 64 bands: byte 7 selects the count and the frame size must match.
    for (uint8_t count : {uint8_t(16), uint8_t(64)}) {
        packet[7] = count;
        for (uint8_t i = 0; i < count; ++i) put16(packet + 38 + 2 * i, uint16_t(i * 64));
        assert(!decode(packet, size, frame)); // wrong size for this count
        assert(decode(packet, frameSize(count), frame));
        assert(frame.band_count == count && frame.bands[count - 1] == (count - 1) * 64);
        assert(!decode(packet, frameSize(count) + 2, frame));
        put16(packet + 38 + 2 * (count - 1), 4097);
        assert(!decode(packet, frameSize(count), frame));
    }
    packet[7] = 48; assert(!decode(packet, frameSize(48), frame));
    packet[7] = 0; assert(!decode(packet, 38, frame));
}
