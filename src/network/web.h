#pragma once

#include "ESPAsyncWebServer.h"

class WebServer {
    uint16_t _port;
    AsyncWebServer _server;

public:
    explicit WebServer(uint16_t port = 80);

    void begin();

    inline void on(const char *uri, WebRequestMethodComposite method, ArRequestHandlerFunction onRequest) {
        _server.on(uri, method, onRequest);
    }

    inline void add_handler(AsyncWebHandler *handler) { _server.addHandler(handler); }
};