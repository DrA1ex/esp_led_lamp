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
            auto hsv = _app.preset().color_effect == ColorEffectEnum::SOLID
                       ? CHSV(_app.preset().speed, _app.preset().scale, 255)
                       : CHSV(255, 255, 225);

            CRGB color{};
            hsv2rgb_rainbow(hsv, color);

            return response_with_json(request, JsonPropListT{
                    {"status", "ok"},
                    {"value",  static_cast<uint32_t>(color) & 0xffffff},

                    {"hue",    hsv.hue},
                    {"sat",    hsv.sat},
                    {"bri",    hsv.val},
            });
        }

        PresetConfig *preset = nullptr;
        if (_app.preset().color_effect == ColorEffectEnum::SOLID) {
            preset = &_app.preset();
        } else {
            for (int i = 0; i < _app.preset_configs.count; ++i) {
                if (_app.preset_configs.presets[i].color_effect == ColorEffectEnum::SOLID) {
                    _app.change_preset(i);
                    preset = &_app.preset_configs.presets[i];
                    break;
                }
            }

            if (preset == nullptr) {
                return response_with_json_status(request, "error");
            }
        }


        auto new_color = CRGB(request->arg("value").toInt());
        auto hsv = rgb2hsv_approximate(new_color);
        preset->speed = hsv.hue;
        preset->scale = hsv.sat;

        _app.load();

        response_with_json(request, {
                {"status", "ok"},
                {"hue", hsv.hue},
                {"sat", hsv.sat},
                {"bri", hsv.val},
        });
    });
}
