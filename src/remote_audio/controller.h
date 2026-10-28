#pragma once
#include <Arduino.h>
#include "audio/base.h"
#include "config.h"
#include "remote_audio/timeline.h"

// Does not modify Application::config or persisted preset storage.
class RemoteAudioController {
public:
    struct Overrides {
        AudioConfig audio;
        PresetConfig preset;
    };
private:
    RemoteAudio::Timeline _timeline;
    Overrides _overrides;
    uint32_t _owner = 0;
    uint32_t _session = 0;
    uint32_t _last_frame = 0;
    uint32_t _last_control = 0;
    bool _playing = false;
    uint16_t _pulse[3] = {};
    uint32_t _last_pulse_tick = 0;

public:
    RemoteAudio::Timeline &timeline() { return _timeline; }
    const RemoteAudio::Timeline &timeline() const { return _timeline; }
    Overrides &overrides() { return _overrides; }
    const Overrides &overrides() const { return _overrides; }
    uint32_t owner() const { return _owner; }
    uint32_t session() const { return _session; }
    bool owns(uint32_t client, uint32_t session) const {
        return _owner != 0 && _owner == client && _session == session && session != 0;
    }
    bool begin(uint32_t client, uint32_t session, const AudioConfig &audio, const PresetConfig &preset) {
        if (!client || !session || (_owner && _owner != client)) return false;
        // A new hello, including reconnect using a new session, invalidates old frames.
        _timeline.reset();
        memset(_pulse, 0, sizeof(_pulse)); _last_pulse_tick = 0;
        _owner = client; _session = session;
        _overrides.audio = audio;
        _overrides.audio.enabled = true;
        _overrides.preset = preset;
        _last_frame = millis();
        _last_control = millis();
        _playing = false;
        return true;
    }
    bool start(uint32_t client, uint32_t session) {
        if (!owns(client, session) || !_timeline.clockValid()) return false;
        _playing = true;
        _last_frame = millis();
        _last_control = _last_frame;
        return true;
    }
    // Stop rendering without dropping a connected browser's control lease.
    void pause() {
        memset(_pulse, 0, sizeof(_pulse)); _last_pulse_tick = 0;
        // Clear pending data without losing clock sync across browser pause.
        const bool synced = _timeline.clockValid();
        const int32_t offset = _timeline.offset();
        const uint32_t rtt = _timeline.stats().rtt_ms;
        _timeline.reset();
        if (synced) _timeline.setClock(offset, rtt);
        _playing = false;
        _last_control = millis();
    }
    void stop() {
        pause();
        _session = 0; _owner = 0;
    }
    void disconnected(uint32_t client) {
        if (_owner == client) stop();
    }
    bool playing() const { return _playing; }
    const uint16_t *pulses() const { return _pulse; }
    bool adoptSession(uint32_t client, uint32_t session) {
        if (_owner != client || !session) return false;
        const bool synced = _timeline.clockValid();
        const int32_t offset = _timeline.offset();
        const uint32_t rtt = _timeline.stats().rtt_ms;
        _timeline.reset();
        if (synced) _timeline.setClock(offset, rtt);
        _session = session;
        _playing = false;
        _last_control = millis();
        return true;
    }
    bool receive(uint32_t client, const RemoteProtocol::Frame &frame) {
        if (!_playing || !owns(client, frame.session)) return false;
        const uint32_t now = millis();
        const bool accepted = _timeline.push(frame, now);
        if (accepted) _last_frame = now;
        return accepted;
    }
    void activity() { _last_control = millis(); }
    // Release idle ownership even if a client never sends start/stop.
    void poll() {
        const uint32_t now = millis();
        if (!_owner) return;
        if (_playing) {
            if (uint32_t(now - _last_frame) > RemoteAudio::STREAM_TIMEOUT_MS &&
                _timeline.occupancy() == 0) pause();
        } else if (uint32_t(now - _last_control) > 5000) stop();
    }
    bool sample(uint32_t now) {
        poll();
        if (!_playing || !_timeline.sample(now)) return false;
        const auto &frame = _timeline.output();
        const uint32_t elapsed = _last_pulse_tick ? uint32_t(now - _last_pulse_tick) : 16u;
        _last_pulse_tick = now;
        // Each onset is consumed only once by Timeline; decay envelope ~100 ms.
        const uint32_t steps = elapsed > 80 ? 5 : (elapsed + 15) / 16;
        for (uint8_t i = 0; i < 3; ++i) {
            for (uint8_t j = 0; j < steps; ++j)
                _pulse[i] = uint32_t(_pulse[i]) * 218 / 256;
            if (frame.onset_mask & (1u << i)) {
                if (frame.onset > _pulse[i]) _pulse[i] = frame.onset;
            }
        }
        return true;
    }
};

class RemoteSignalProvider final : public SignalProvider {
public:
    enum Kind : uint8_t { WAVE, SPECTRUM, PARAMETRIC };
private:
    const RemoteAudioController &_controller;
    Kind _kind;
    uint16_t _width;
public:
    RemoteSignalProvider(const RemoteAudioController &controller, Kind kind, uint16_t width) :
            _controller(controller), _kind(kind), _width(width) {}
    void tick() override {}
    uint16_t max_value() const override { return RemoteProtocol::MAX_LEVEL; }
    uint16_t get(uint16_t index, uint8_t) const override {
        if (!_controller.playing()) return 0;
        if (_kind == WAVE) return _controller.timeline().wave(index);
        if (_kind == PARAMETRIC) return _controller.timeline().output().rms;
        if (_width < 2) return _controller.timeline().output().bands[0];
        const uint32_t bandpos = uint32_t(index) * (RemoteProtocol::BAND_COUNT - 1) * 256 / (_width - 1);
        const uint16_t left = bandpos / 256;
        const uint8_t frac = bandpos & 255;
        if (left >= RemoteProtocol::BAND_COUNT - 1) return _controller.timeline().output().bands[31];
        return RemoteAudio::mix(_controller.timeline().output().bands[left],
                                _controller.timeline().output().bands[left + 1], frac);
    }
    uint16_t delta() const override { return 0; } // interpolated in Timeline already
    uint16_t update_interval() const override { return 25; }
    void set_gain(uint16_t) override {}
    void set_gate(uint16_t) override {}
};
