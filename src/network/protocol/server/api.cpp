#include "api.h"

#include <ArduinoJson.h>

#include "utils/math.h"


ApiWebServer::ApiWebServer(Application &application, const char *path) : _app(application), _path(path) {}

void ApiWebServer::begin(WebServer &server) {
    server.on((_path + String("/power")).c_str(), HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!request->hasArg("value")) {
            D_PRINT("Request power status");
            return response_with_json(request, JsonPropListT{
                    {"status", "ok"},
                    {"value",  _app.config.power ? 1 : 0},
            });
        }

        bool enabled = request->arg("value") == "1";
        _app.set_power(enabled);

        response_with_json_status(request, "ok");
    });

    server.on((_path + String("/brightness")).c_str(), HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!request->hasArg("value")) {
            D_PRINT("Request brightness status");
            return response_with_json(request, JsonPropListT{
                    {"status", "ok"},
                    {"value",  map16(_app.config.max_brightness, 255, 100)},
            });
        }

        auto new_brightness = map16(request->arg("value").toInt(), 100, 255);

        _app.config.max_brightness = new_brightness;
        _app.load();

        response_with_json_status(request, "ok");
    });

    server.on((_path + String("/color")).c_str(), HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!request->hasArg("value")) {
            D_PRINT("Request color status");
            auto color = _app.current_color();

            return response_with_json(request, JsonPropListT{
                    {"status", "ok"},
                    {"value",  color}
            });
        }


        _app.change_color(request->arg("value").toInt());
        response_with_json_status(request, "ok");
    });
}
