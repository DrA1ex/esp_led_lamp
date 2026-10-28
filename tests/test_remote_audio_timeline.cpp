#include <cassert>
#include <cstdint>
#include "../src/remote_audio/timeline.h"

static RemoteProtocol::Frame frame(uint32_t seq, uint32_t timestamp, uint16_t level) {
    RemoteProtocol::Frame f{};
    f.session = 123;
    f.sequence = seq;
    f.presentation_ms = timestamp;
    f.rms = level;
    f.onset_mask = seq == 1 ? 1 : seq == 2 ? 2 : 0;
    f.onset = level;
    for (unsigned i = 0; i < RemoteProtocol::BAND_COUNT; ++i) f.bands[i] = level;
    return f;
}
int main() {
    RemoteAudio::Timeline timeline;
    assert(!timeline.clockValid());
    assert(!timeline.push(frame(1, 100, 1000), 200));
    timeline.setClock(100, 12);
    assert(timeline.offset() == 100 && timeline.clockValid());
    // Out-of-order input should be sorted by timestamp.
    assert(timeline.push(frame(1, 100, 1000), 190));
    assert(timeline.push(frame(3, 150, 3000), 190));
    assert(timeline.push(frame(2, 125, 2000), 190));
    assert(timeline.occupancy() == 3);
    assert(!timeline.push(frame(2, 125, 2000), 190));
    assert(timeline.stats().duplicate == 1);
    assert(!timeline.sample(199));
    assert(timeline.sample(200));
    assert(timeline.output().rms == 1000);
    assert(timeline.output().onset_mask == 1);
    assert(timeline.sample(212));
    assert(timeline.output().rms > 1400 && timeline.output().rms < 1600);
    assert(timeline.output().onset_mask == 0);
    assert(timeline.sample(225));
    assert(timeline.output().rms == 2000);
    assert(timeline.output().onset_mask == 2);
    assert(!timeline.push(frame(4, 125, 1500), 225));
    assert(timeline.sample(250));
    assert(timeline.output().rms == 3000);
    assert(timeline.wave(0) != 0);
    // Timeout and new sessions reset the entire buffer and dedupe window.
    timeline.reset();
    assert(timeline.occupancy() == 0 && !timeline.clockValid());
    timeline.setClock(-40, 1);
    assert(timeline.push(frame(0xFFFFFFFF, 1000, 500), 950));
    assert(timeline.push(frame(0, 1025, 600), 950)); // wrap-safe sequence
    assert(!timeline.push(frame(0, 1025, 600), 950));
    // Reject absurd future and stale frames.
    assert(!timeline.push(frame(5, 5000, 500), 950));
    assert(!timeline.push(frame(6, 100, 500), 950));
}
