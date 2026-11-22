#include "web.h"

class WebLogger : public AsyncWebHandler {
    bool canHandle(AsyncWebServerRequest *request) const override {
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

void WebServer::begin(FS *fs) {
    _fs = fs;

    _server.serveStatic("/", *_fs, "/").setDefaultFile("index.html").setCacheControl("no-cache");

    _server.begin();

    D_WRITE("Web server listening on port: ");
    D_PRINT(_port);
}

void WebAuthHandler::handleRequest(AsyncWebServerRequest *request) {
    D_PRINTF("Reject request from: %s\n", request->client()->remoteIP().toString().c_str());

    request->redirect("https://google.com");
}

bool WebAuthHandler::canHandle(AsyncWebServerRequest *request) const {
    const auto ip = request->client()->remoteIP();
    bool is_local = ip[0] == 192 && ip[1] == 168;
    bool auth_required = _allow_local ? !is_local : true;

    return auth_required && !request->authenticate(AUTH_USER, AUTH_PASSWORD);
}
