#pragma once
#include "constants.h"
#if REMOTE_AUDIO == ENABLED
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "network/web.h"
#include "remote_audio/controller.h"
#include "application.h"

class RemoteAudioSocket {
    AsyncWebSocket _ws{"/audio"};
    RemoteAudioController &_audio;
    Application &_app;
    uint32_t _lab_client = 0;
    uint8_t _fragments[512] = {};
    uint32_t _fragment_client = 0;
    uint8_t _fragment_opcode = 0;
    size_t _fragment_length = 0;

    // TCP callback is a producer; 20 ms service_loop is the consumer.
    // Keep JSON allocation, session changes and WS replies outside TCP callbacks.
    static constexpr uint8_t QUEUE_CAPACITY = 4; // 3 slots usable (SPSC)
    struct PendingMessage {
        uint32_t client = 0;
        uint16_t length = 0;
        uint8_t opcode = 0;
        uint8_t payload[512] = {};
    };
    PendingMessage _inbox[QUEUE_CAPACITY] = {};
    volatile uint8_t _head = 0;
    volatile uint8_t _tail = 0;
    volatile uint32_t _pending_disconnect = 0;
    uint32_t _last_cleanup = 0;
    uint32_t _queue_drops = 0;

    void enqueue(uint32_t client, uint8_t opcode, const uint8_t *data, size_t length);

    void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                 AwsEventType type, void *arg, uint8_t *data, size_t len);
    void onMessage(uint32_t client, uint8_t opcode, const uint8_t *data, size_t len);
    void textMessage(uint32_t client, const uint8_t *data, size_t len);
    void reply(uint32_t client, const char *type, const char *reason = nullptr);
    bool configure(JsonDocument &doc);

public:
    RemoteAudioSocket(RemoteAudioController &controller, Application &app) : _audio(controller), _app(app) {}
    void begin(WebServer &server);
    void poll();
};
#endif
