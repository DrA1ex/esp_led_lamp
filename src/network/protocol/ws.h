#pragma once

#include "AsyncWebSocket.h"

#include "base.h"
#include "../web.h"

class WebSocketServer : ServerBase {
    const char *_path;
    AsyncWebSocket _ws;

public:
    explicit WebSocketServer(AppConfig &config, const char *path = "/ws");

    void begin(WebServer &server);

    void handle_incoming_data() override;

protected:
    void on_event(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
};
