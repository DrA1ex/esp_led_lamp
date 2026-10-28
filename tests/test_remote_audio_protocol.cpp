// Native, dependency-free protocol regression tests:
// g++ -std=c++17 -Wall -Wextra -Werror tests/test_remote_audio_protocol.cpp -o /tmp/remote_audio_protocol_test && /tmp/remote_audio_protocol_test
#include <cassert>
#include <cstdint>
#include <cstring>
#include "../src/remote_audio/protocol.h"

static void put16(uint8_t *p, uint16_t n) { p[0] = uint8_t(n); p[1] = uint8_t(n >> 8); }
static void put32(uint8_t *p, uint32_t n) { put16(p, uint16_t(n)); put16(p + 2, uint16_t(n >> 16)); }

int main() {
    using namespace RemoteProtocol;
    uint8_t packet[FRAME_SIZE] = {};
    std::memcpy(packet, "ELAF", 4);
    packet[4] = 1; packet[5] = 1; packet[7] = 32;
    put32(packet + 8, 0x01234567);
    put32(packet + 12, 0xFFFFFFFF);
    put32(packet + 16, 0xFFFFFFF0);
    put16(packet + 20, 4096);
    put16(packet + 38, 3000);
    put16(packet + 100, 4096);
    Frame frame;
    assert(decode(packet, sizeof(packet), frame));
    assert(frame.session == 0x01234567 && frame.sequence == 0xFFFFFFFF);
    assert(frame.presentation_ms == 0xFFFFFFF0);
    assert(frame.rms == 4096 && frame.bands[0] == 3000 && frame.bands[31] == 4096);
    assert(!decode(packet, sizeof(packet) - 1, frame));
    packet[0] = 'X'; assert(!decode(packet, sizeof(packet), frame)); packet[0] = 'E';
    packet[4] = 2; assert(!decode(packet, sizeof(packet), frame)); packet[4] = 1;
    packet[6] = 2; assert(!decode(packet, sizeof(packet), frame)); packet[6] = 1;
    packet[7] = 31; assert(!decode(packet, sizeof(packet), frame)); packet[7] = 32;
    packet[32] = 8; assert(!decode(packet, sizeof(packet), frame)); packet[32] = 0;
    packet[37] = 1; assert(!decode(packet, sizeof(packet), frame)); packet[37] = 0;
    put16(packet + 20, 4097); assert(!decode(packet, sizeof(packet), frame));
    put16(packet + 20, 4096);
    put16(packet + 100, 4097); assert(!decode(packet, sizeof(packet), frame));
    put16(packet + 100, 4096);
    put32(packet + 8, 0); assert(!decode(packet, sizeof(packet), frame));
    put32(packet + 8, 0x01234567);
    assert(decode(packet, sizeof(packet), frame));
}
