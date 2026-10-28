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
    uint8_t _fragments[512] = {};
    uint32_t _fragment_client = 0;
    uint8_t _fragment_opcode = 0;
    size_t _fragment_length = 0;

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
