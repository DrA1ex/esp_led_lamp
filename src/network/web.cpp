#include "web.h"

#include "LittleFS.h"

WebServer::WebServer(uint16_t port) : _port(port), _server(port) {}

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