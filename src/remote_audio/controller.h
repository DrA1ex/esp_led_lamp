#pragma once
#include <Arduino.h>
#include "audio/base.h"
#include "audio/transfer.h"
#include "config.h"
#include "remote_audio/timeline.h"
#include "remote_audio/reactive_math.h"
#include "remote_audio/viz_engine.h"

// Does not modify Application::config or persisted preset storage.
class RemoteAudioController {
public:
    // Lab-only reaction tuning. Zero values reproduce the stock firmware
    // AudioEffectManager behaviour, so receivers ignoring them stay correct.
    struct Tuning {
        RemoteReactive::ControlSource param_source = RemoteReactive::ControlSource::LOUDNESS;
        uint8_t speed_floor = 0;     // 0..255, Speed Control lower bound
        uint8_t spectrum_shape = 0;  // 0..100 %, brighter Wave/Spectrum curve
        uint8_t spectrum_onset = 0;  // 0..100 %, onset accents in Wave/Spectrum
        AudioWindow window;          // input window / gate before audio min..max
        RemoteReactive::Origin origin;
        uint8_t viz_color = 0;       // visualizations: 0 palette colours, 1 mask the colour effect
        Viz::Settings viz;           // visualizations: mirror, auto-level, decay / motion %
    };
    struct Overrides {
        AudioConfig audio;
        PresetConfig preset;
        Tuning tuning;
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
        _overrides.tuning = Tuning{};
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
    uint16_t controlValue() const {
        return RemoteReactive::controlValue(_timeline.output(), _pulse, _overrides.tuning.param_source);
    }
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
        if (_kind == WAVE) {
            // One column per 25 ms sample, like the stock VolumeAnalyzer<.., WIDTH>.
            // Long strips stretch the history instead of repeating it.
            constexpr uint16_t N = RemoteAudio::WAVE_HISTORY;
            const uint16_t age = _width > N ? uint32_t(index) * (N - 1) / (_width - 1) : index;
            return _controller.timeline().wave(age);
        }
        if (_kind == PARAMETRIC) return _controller.controlValue();
        const auto &output = _controller.timeline().output();
        const uint16_t *bands = output.bands;
        const uint16_t B = output.band_count;
        if (_width < 2) return bands[0];
        if (_width < B) {
            // Fewer columns than bands (short strips, centered halves): average
            // each column's band range instead of skipping bands entirely.
            const uint16_t first = uint32_t(index) * B / _width;
            uint16_t last = uint32_t(index + 1) * B / _width;
            if (last > B) last = B;
            if (last <= first) return bands[first < B ? first : B - 1];
            uint32_t sum = 0;
            for (uint16_t i = first; i < last; ++i) sum += bands[i];
            return sum / (last - first);
        }
        const uint32_t bandpos = uint32_t(index) * (B - 1) * 256 / (_width - 1);
        const uint16_t left = bandpos / 256;
        const uint8_t frac = bandpos & 255;
        if (left >= B - 1) return bands[B - 1];
        return RemoteAudio::mix(bands[left], bands[left + 1], frac);
    }
    uint16_t delta() const override { return 0; } // interpolated in Timeline already
    uint16_t update_interval() const override { return 25; }
    void set_gain(uint16_t) override {}
    void set_gate(uint16_t) override {}
};
