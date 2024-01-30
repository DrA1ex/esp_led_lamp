#include "web.h"

#include "LittleFS.h"

class WebLogger : public AsyncWebHandler {
    bool canHandle(AsyncWebServerRequest *request) override {
        D_PRINTF("WebServer: %s -> %s %s\n", request->client()->remoteIP().toString().c_str(),
                 request->methodToString(), request->url().c_str());

        return false;
    }
};

WebServer::WebServer(uint16_t port) : _port(port), _server(port) {
#if DEBUG
    _server.addHandler(new WebLogger());
#endif
}

void WebServer::begin() {
    if (!LittleFS.begin()) {
        D_PRINT("Unable to initialize FS");
    }

    _server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/index.html", "text/html");
    });

    _server.serveStatic("/", LittleFS, "/");

    _server.begin();

    D_WRITE("Web server listening on port: ");
    D_PRINT(_port);
}