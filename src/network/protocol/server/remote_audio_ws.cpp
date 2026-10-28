#include "constants.h"
#if REMOTE_AUDIO == ENABLED
#include "network/protocol/server/remote_audio_ws.h"
#include <ArduinoJson.h>
#include <string.h>
#include "fx/fx.h"

void RemoteAudioSocket::begin(WebServer &server) {
    _ws.onEvent([this](AsyncWebSocket *s, AsyncWebSocketClient *c,
                       AwsEventType type, void *arg, uint8_t *data, size_t len) {
        onEvent(s, c, type, arg, data, len);
    });
    server.add_handler(&_ws);
}
void RemoteAudioSocket::poll() {
    // Process events on the Arduino loop, never from AsyncTCP callbacks.
    const uint32_t disconnected = _pending_disconnect;
    _pending_disconnect = 0;
    if (disconnected) {
        _audio.disconnected(disconnected);
        if (_lab_client == disconnected) _lab_client = 0;
    }
    uint8_t budget = QUEUE_CAPACITY - 1;
    while (_tail != _head && budget--) {
        auto &message = _inbox[_tail];
        // Never allow already queued messages to resurrect a disconnected session.
        if (message.client != disconnected)
            onMessage(message.client, message.opcode, message.payload, message.length);
        _tail = (_tail + 1) % QUEUE_CAPACITY;
    }
    _audio.poll();
    const uint32_t now = millis();
    if (uint32_t(now - _last_cleanup) >= 1000) {
        _last_cleanup = now;
        _ws.cleanupClients();
    }
}
void RemoteAudioSocket::enqueue(uint32_t client, uint8_t opcode,
                                const uint8_t *data, size_t length) {
    if (length > sizeof(_inbox[0].payload)) { ++_queue_drops; return; }
    const uint8_t next = (_head + 1) % QUEUE_CAPACITY;
    if (next == _tail) { ++_queue_drops; return; }
    auto &message = _inbox[_head];
    message.client = client;
    message.opcode = opcode;
    message.length = static_cast<uint16_t>(length);
    memcpy(message.payload, data, length);
    _head = next; // publish only after copying a complete message
}
void RemoteAudioSocket::reply(uint32_t client, const char *type, const char *reason) {
    char response[180];
    if (reason) snprintf(response, sizeof(response),
                         "{\"type\":\"%s\",\"reason\":\"%s\"}", type, reason);
    else snprintf(response, sizeof(response),
                  "{\"type\":\"%s\",\"session\":%lu}", type,
                  static_cast<unsigned long>(_audio.session()));
    _ws.text(client, response);
}
void RemoteAudioSocket::onEvent(AsyncWebSocket *, AsyncWebSocketClient *client,
                                AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_DISCONNECT || type == WS_EVT_ERROR) {
        // Defer all controller changes to poll(); only publish an ID here.
        if (_fragment_client == client->id()) {
            _fragment_client = 0;
            _fragment_length = 0;
        }
        if (_audio.owner() == client->id()) _pending_disconnect = client->id();
        return;
    }
    if (type != WS_EVT_DATA || !arg) return;
    const auto *info = static_cast<AwsFrameInfo *>(arg);
    // Continuation frames carry opcode 0; use the opcode of their first fragment.
    const uint8_t opcode = info->opcode == WS_CONTINUATION &&
                           _fragment_client == client->id() ? _fragment_opcode : info->opcode;
    if (opcode != WS_TEXT && opcode != WS_BINARY) return;
    const size_t maximum = opcode == WS_BINARY ? RemoteProtocol::FRAME_SIZE : sizeof(_fragments);
    // WebSocket frame fragments are assembled in one statically bounded scratch area.
    // The owner is single-client; concurrent fragmented messages are rejected.
    if (info->num == 0 && info->index == 0) {
        if (_fragment_client && _fragment_client != client->id()) return;
        _fragment_client = client->id();
        _fragment_opcode = opcode;
        _fragment_length = 0;
    }
    if (_fragment_client != client->id() || _fragment_opcode != opcode ||
        len > maximum || _fragment_length > maximum - len) {
        if (_fragment_client == client->id()) {
            _fragment_client = 0;
            _fragment_length = 0;
        }
        return;
    }
    memcpy(_fragments + _fragment_length, data, len);
    _fragment_length += len;
    if (info->final && info->index + len == info->len) {
        enqueue(client->id(), _fragment_opcode, _fragments, _fragment_length);
        _fragment_client = 0;
        _fragment_length = 0;
    }
}
void RemoteAudioSocket::onMessage(uint32_t client, uint8_t opcode, const uint8_t *data, size_t len) {
    if (opcode == WS_BINARY) {
        RemoteProtocol::Frame frame;
        if (RemoteProtocol::decode(data, len, frame)) _audio.receive(client, frame);
    } else if (opcode == WS_TEXT) {
        textMessage(client, data, len);
    }
}
bool RemoteAudioSocket::configure(JsonDocument &doc) {
    auto next = _audio.overrides();
    auto uint8field = [&](const char *name, uint8_t &value, int upper) {
        if (doc[name].isNull()) return true;
        if (!doc[name].is<int>() && !doc[name].is<unsigned int>()) return false;
        int v = doc[name].as<int>();
        if (v < 0 || v > upper) return false;
        value = static_cast<uint8_t>(v);
        return true;
    };
    uint8_t audio_fx = static_cast<uint8_t>(next.audio.effect);
    const bool lab = (_lab_client == _audio.owner());
    if (lab) audio_fx += 1;
    uint8_t color_fx = static_cast<uint8_t>(next.preset.color_effect);
    uint8_t brightness_fx = static_cast<uint8_t>(next.preset.brightness_effect);
    uint8_t palette = static_cast<uint8_t>(next.preset.palette);
    if (!uint8field("audio_effect", audio_fx, AudioEffects.config().count - (lab ? 0 : 1)) ||
        !uint8field("audio_min", next.audio.min, 255) ||
        !uint8field("audio_max", next.audio.max, 255) ||
        !uint8field("color_effect", color_fx, ColorEffects.config().count - 1) ||
        !uint8field("brightness_effect", brightness_fx, BrightnessEffects.config().count - 1) ||
        !uint8field("palette", palette, Palettes.count - 1) ||
        !uint8field("speed", next.preset.speed, 255) ||
        !uint8field("scale", next.preset.scale, 255) ||
        !uint8field("light", next.preset.light, 255)) return false;
    if (next.audio.min > next.audio.max) return false;
    next.audio.effect = static_cast<AudioEffectEnum>(lab && audio_fx ? audio_fx - 1 : audio_fx);
    next.audio.enabled = !(lab && audio_fx == 0);
    if (!doc["audio_enabled"].isNull()) {
        if (!doc["audio_enabled"].is<bool>()) return false;
        next.audio.enabled = doc["audio_enabled"].as<bool>();
    }
    next.preset.color_effect = static_cast<ColorEffectEnum>(color_fx);
    next.preset.brightness_effect = static_cast<BrightnessEffectEnum>(brightness_fx);
    next.preset.palette = static_cast<PaletteEnum>(palette);
    _audio.overrides() = next;
    return true;
}
void RemoteAudioSocket::textMessage(uint32_t client, const uint8_t *data, size_t len) {
    JsonDocument doc;
    if (deserializeJson(doc, data, len)) { reply(client, "error", "invalid_json"); return; }
    const char *type = doc["type"] | "";
    uint32_t session = doc["session"] | uint32_t(0);

    if (strcmp(type, "hello") == 0) {
        if ((doc["version"] | 0) != 1) {
            reply(client, "error", "incompatible_version");
            return;
        }
        // The bundled WASM lab sends hello without a session and establishes
        // the actual session in start(session_id). Standard RFC clients use
        // hello(session). Both are supported without weakening client ownership.
        const bool lab = !session && strcmp(doc["client"] | "", "esp-led-wasm-lab") == 0;
        if (!session && !lab) { reply(client, "error", "missing_session"); return; }
        if (!session) session = 1; // temporary owner marker, replaced on start
        if (!_audio.begin(client, session, _app.config.audio_config, _app.preset())) {
            reply(client, "error", "busy");
            return;
        }
        _lab_client = lab ? client : 0;
        char response[240];
        snprintf(response, sizeof(response),
                 "{\"type\":\"hello_ack\",\"version\":1,\"session\":%lu,"
                 "\"width\":%u,\"height\":%u,\"bands\":32,\"band_counts\":[32],"
                 "\"fps\":60,\"capabilities\":[\"sync\",\"clock\",\"config\",\"status\"]}",
                 static_cast<unsigned long>(session), unsigned(WIDTH), unsigned(HEIGHT));
        _ws.text(client, response);
        return;
    }
    const bool lab = (_lab_client == client && client && _audio.owner() == client);
    if (lab) session = _audio.session();
    if (!_audio.owns(client, session)) {
        reply(client, "error", "invalid_session");
        return;
    }
    _audio.activity();

    if (strcmp(type, "sync") == 0) {
        const uint32_t t0 = doc["t0"] | uint32_t(0);
        const uint32_t t1 = millis();
        const uint32_t id = doc["id"] | uint32_t(0);
        char response[180];
        snprintf(response, sizeof(response),
                 "{\"type\":\"sync_ack\",\"session\":%lu,\"id\":%lu,\"t0\":%lu,"
                 "\"t1\":%lu,\"t2\":%lu}",
                 static_cast<unsigned long>(session), static_cast<unsigned long>(id),
                 static_cast<unsigned long>(t0), static_cast<unsigned long>(t1),
                 static_cast<unsigned long>(millis()));
        _ws.text(client, response);
    } else if (strcmp(type, "clock") == 0) {
        if (doc["offset_ms"].isNull() || doc["offset_ms"].is<const char *>()) {
            reply(client, "error", "invalid_clock");
        } else {
            const double raw = doc["offset_ms"].as<double>();
            if (raw < -2147483648.0 || raw > 2147483647.0 || raw != raw) {
                reply(client, "error", "invalid_clock");
            } else {
                const int32_t offset = static_cast<int32_t>(raw);
                const double raw_rtt = doc["rtt_ms"] | 0.0;
                if (raw_rtt < 0 || raw_rtt > 10000 || raw_rtt != raw_rtt) {
                    reply(client, "error", "invalid_rtt");
                    return;
                }
                _audio.timeline().setClock(offset, static_cast<uint32_t>(raw_rtt));
                reply(client, "clock_ack");
            }
        }
    } else if (strcmp(type, "config") == 0) {
        if (configure(doc)) reply(client, "config_ack");
        else reply(client, "error", "invalid_config");
    } else if (strcmp(type, "start") == 0) {
        const uint32_t next = lab ? (doc["session_id"] | uint32_t(0)) : session;
        if (lab && !_audio.adoptSession(client, next)) {
            reply(client, "error", "invalid_session");
        } else if (_audio.start(client, next)) {
            reply(client, "start_ack");
        } else reply(client, "error", "clock_not_set");
    } else if (strcmp(type, "stop") == 0) {
        if (lab) _audio.pause();
        else _audio.stop();
        reply(client, "stop_ack");
    } else if (strcmp(type, "status") == 0) {
        const auto &stat = _audio.timeline().stats();
        char response[480];
        snprintf(response, sizeof(response),
                 "{\"type\":\"status\",\"session\":%lu,\"playing\":%s,"
                 "\"received\":%lu,\"received_frames\":%lu,\"dropped\":%lu,\"duplicates\":%lu,"
                 "\"stale\":%lu,\"overflow\":%lu,\"frame_age_ms\":%lu,"
                 "\"clock_offset_ms\":%ld,\"rtt_ms\":%lu,\"buffer\":%u,\"buffer_occupancy\":%u,""\"heap\":%u,\"heap_max_block\":%u,\"heap_fragmentation\":%u,\"queue_drops\":%lu}",
                 static_cast<unsigned long>(session), _audio.playing() ? "true" : "false",
                 static_cast<unsigned long>(stat.received), static_cast<unsigned long>(stat.received),
                 static_cast<unsigned long>(stat.dropped), static_cast<unsigned long>(stat.duplicate),
                 static_cast<unsigned long>(stat.stale), static_cast<unsigned long>(stat.overflow),
                 static_cast<unsigned long>(stat.frame_age_ms),
                 static_cast<long>(static_cast<int32_t>(stat.clock_offset_ms)),
                 static_cast<unsigned long>(stat.rtt_ms),
                 unsigned(_audio.timeline().occupancy()), unsigned(_audio.timeline().occupancy()),
                 unsigned(ESP.getFreeHeap()), unsigned(ESP.getMaxFreeBlockSize()),
                 unsigned(ESP.getHeapFragmentation()), static_cast<unsigned long>(_queue_drops));
        _ws.text(client, response);
    } else reply(client, "error", "unknown_command");
}
#endif
