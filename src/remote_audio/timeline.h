#pragma once
#include "protocol.h"
#include <stdint.h>
#include <string.h>

// Fixed-capacity, allocation-free, transport-independent presentation timeline.
// All comparisons operate on uint32 millisecond clocks modulo 2^32.
namespace RemoteAudio {
constexpr uint8_t BUFFER_CAPACITY = 24;
constexpr uint32_t STREAM_TIMEOUT_MS = 300;
constexpr uint32_t WAVE_PERIOD_MS = 25;
inline int32_t distance(uint32_t a, uint32_t b) { return static_cast<int32_t>(a - b); }
inline uint16_t mix(uint16_t a, uint16_t b, uint8_t frac) {
    return static_cast<uint16_t>((uint32_t(a) * (255 - frac) + uint32_t(b) * frac + 127) / 255);
}

struct Statistics {
    uint32_t received = 0;
    uint32_t dropped = 0;
    uint32_t duplicate = 0;
    uint32_t stale = 0;
    uint32_t overflow = 0;
    uint32_t frame_age_ms = 0;
    uint32_t rtt_ms = 0;
    uint32_t clock_offset_ms = 0; // signed bits, interpreted as int32
};

class Timeline {
    struct Queued {
        RemoteProtocol::Frame frame;
        uint32_t local_time;
    };
    Queued _frames[BUFFER_CAPACITY] = {};
    uint8_t _size = 0;
    bool _current_valid = false;
    Queued _current = {};
    RemoteProtocol::Frame _output = {};
    uint16_t _wave[64] = {};
    uint8_t _wave_head = 0;
    uint32_t _wave_at = 0;
    bool _has_sequence = false;
    uint32_t _newest_sequence = 0;
    uint32_t _seen_sequence_mask = 0;
    int32_t _offset_ms = 0;
    bool _clock_valid = false;
    Statistics _stats = {};

public:
    void reset() {
        _size = 0; _current_valid = false; _output = RemoteProtocol::Frame{};
        memset(_wave, 0, sizeof(_wave)); _wave_head = 0; _wave_at = 0;
        _has_sequence = false; _newest_sequence = 0; _seen_sequence_mask = 0;
        _offset_ms = 0; _clock_valid = false; _stats = Statistics{};
    }
    bool clockValid() const { return _clock_valid; }
    int32_t offset() const { return _offset_ms; }
    void setClock(int32_t offset_ms, uint32_t rtt_ms) {
        // Rebase the still-buffered frames rather than changing times mid-stream.
        const int32_t adjust = offset_ms - _offset_ms;
        for (uint8_t i = 0; i < _size; ++i) _frames[i].local_time += adjust;
        _offset_ms = offset_ms;
        _clock_valid = true;
        _stats.clock_offset_ms = static_cast<uint32_t>(offset_ms);
        _stats.rtt_ms = rtt_ms;
    }
    const Statistics &stats() const { return _stats; }
    uint8_t occupancy() const { return _size; }
    const RemoteProtocol::Frame &output() const { return _output; }
    uint16_t wave(uint8_t age) const {
        return _wave[uint8_t((_wave_head + 64 - 1 - (age % 64)) % 64)];
    }
    uint32_t age() const { return _stats.frame_age_ms; }

    bool push(const RemoteProtocol::Frame &frame, uint32_t now) {
        if (!_clock_valid) { ++_stats.dropped; return false; }
        const uint32_t local_time = frame.presentation_ms + static_cast<uint32_t>(_offset_ms);
        const int32_t skew = distance(local_time, now);
        if (skew < -180 || skew > 2000 ||
            (_current_valid && distance(local_time, _current.local_time) <= 0)) {
            ++_stats.dropped; ++_stats.stale; return false;
        }
        // Sliding 32-sequence replay window, permits network reordering.
        if (_has_sequence) {
            const int32_t advance = distance(frame.sequence, _newest_sequence);
            if (advance > 0) {
                _seen_sequence_mask = advance >= 32 ? 1 : (_seen_sequence_mask << advance) | 1;
                _newest_sequence = frame.sequence;
            } else {
                const uint32_t back = uint32_t(-int64_t(advance));
                if (back >= 32 || (_seen_sequence_mask & (uint32_t(1) << back))) {
                    ++_stats.dropped; ++_stats.duplicate; return false;
                }
                _seen_sequence_mask |= uint32_t(1) << back;
            }
        } else {
            _has_sequence = true;
            _newest_sequence = frame.sequence;
            _seen_sequence_mask = 1;
        }

        if (_size == BUFFER_CAPACITY) {
            // Bound memory: discard the oldest pending timestamp first.
            for (uint8_t i = 1; i < _size; ++i) _frames[i - 1] = _frames[i];
            --_size;
            ++_stats.overflow; ++_stats.dropped;
        }
        uint8_t where = _size;
        while (where > 0 && distance(local_time, _frames[where - 1].local_time) < 0) {
            _frames[where] = _frames[where - 1];
            --where;
        }
        _frames[where] = {frame, local_time};
        ++_size;
        ++_stats.received;
        return true;
    }

    bool sample(uint32_t now) {
        uint8_t onset_mask = 0;
        uint16_t onset_strength = 0;
        // Consume each timestamp only once. Onset is a one-tick event, never interpolated.
        while (_size > 0 && distance(now, _frames[0].local_time) >= 0) {
            _current = _frames[0];
            _current_valid = true;
            onset_mask |= _current.frame.onset_mask;
            if (_current.frame.onset > onset_strength) onset_strength = _current.frame.onset;
            for (uint8_t i = 1; i < _size; ++i) _frames[i - 1] = _frames[i];
            --_size;
        }
        if (!_current_valid) return false;

        _output = _current.frame;
        _output.onset_mask = onset_mask;
        _output.onset = onset_strength;
        _stats.frame_age_ms = uint32_t(distance(now, _current.local_time) > 0 ?
                                             distance(now, _current.local_time) : 0);
        if (_size > 0) {
            const int32_t duration = distance(_frames[0].local_time, _current.local_time);
            if (duration > 0) {
                int32_t pos = distance(now, _current.local_time);
                if (pos < 0) pos = 0;
                const uint8_t frac = static_cast<uint8_t>(
                        pos >= duration ? 255 : (uint32_t(pos) * 255) / uint32_t(duration));
                const auto &next = _frames[0].frame;
                _output.rms = mix(_output.rms, next.rms, frac);
                _output.bass = mix(_output.bass, next.bass, frac);
                _output.mid = mix(_output.mid, next.mid, frac);
                _output.high = mix(_output.high, next.high, frac);
                _output.peak = mix(_output.peak, next.peak, frac);
                for (uint8_t i = 0; i < RemoteProtocol::BAND_COUNT; ++i) {
                    _output.bands[i] = mix(_output.bands[i], next.bands[i], frac);
                }
            }
        }
        if (_wave_at == 0) _wave_at = now;
        uint8_t limit = 0;
        while (distance(now, _wave_at) >= 0 && limit++ < 4) {
            _wave[_wave_head++ % 64] = _output.rms;
            _wave_at += WAVE_PERIOD_MS;
        }
        if (distance(now, _wave_at) > 100) _wave_at = now + WAVE_PERIOD_MS;
        return true;
    }
};
} // namespace RemoteAudio
