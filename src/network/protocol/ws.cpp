#include "ws.h"

#include "functional"

#include "debug.h"

WebSocketServer::WebSocketServer(Application &app, const char *path) : ServerBase(app), _path(path), _ws(path) {}


void WebSocketServer::begin(WebServer &server) {
    auto event_handler = std::bind(&WebSocketServer::on_event, this,
                                   std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
                                   std::placeholders::_4, std::placeholders::_5, std::placeholders::_6);

    _ws.onEvent(event_handler);
    server.add_handler(&_ws);

    D_WRITE("WebSocket server listening on path: ");
    D_PRINT(_path);
}

void WebSocketServer::handle_incoming_data() {
    _ws.cleanupClients();
}

void WebSocketServer::on_event(AsyncWebSocket *server,
                               AsyncWebSocketClient *client,
                               AwsEventType type,
                               void *arg,
                               uint8_t *data,
                               size_t len) {

    switch (type) {
        case WS_EVT_CONNECT:
            D_PRINTF("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            break;

        case WS_EVT_DISCONNECT:
            D_PRINTF("WebSocket client #%u disconnected\n", client->id());
            break;

        case WS_EVT_DATA: {
            D_PRINTF("Received WebSocket packet, size: %u\n", len);
            auto response = handle_packet_data(data, len);

            switch (response.type) {
                case ResponseType::CODE:
                    _ws.text(client->id(), response.code_string());
                    break;

                case ResponseType::STRING:
                    _ws.text(client->id(), response.body.str);
                    break;

                case ResponseType::BINARY:
                    _ws.binary(client->id(), response.body.buffer.data, (size_t) response.body.buffer.size);
                    break;
            }
            break;
        }

        default:
            break;
    }
}
