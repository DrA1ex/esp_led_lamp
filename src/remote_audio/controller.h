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
    void stop() {
        _timeline.reset();
        _session = 0; _owner = 0; _playing = false;
    }
    void disconnected(uint32_t client) {
        if (_owner == client) stop();
    }
    bool playing() const { return _playing; }
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
                _timeline.occupancy() == 0) stop();
        } else if (uint32_t(now - _last_control) > 5000) stop();
    }
    bool sample(uint32_t now) {
        poll();
        return _playing && _timeline.sample(now);
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
