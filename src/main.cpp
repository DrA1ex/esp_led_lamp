#include <Arduino.h>

#include "constants.h"
#include "config.h"
#include "led.h"

#include "network/wifi.h"
#include "network/web.h"
#include "network/protocol/udp.h"
#include "network/protocol/ws.h"

Led led(WIDTH, HEIGHT);

Config config;
volatile AppConfig appConfig(config);

WebServer webServer(WEB_PORT);

UdpServer udpServer((AppConfig &) appConfig);
WebSocketServer wsServer((AppConfig &) appConfig);

void setup() {
#ifdef DEBUG
    Serial.begin(115200);
#endif

    led.setPowerLimit(MATRIX_VOLTAGE, CURRENT_LIMIT);
    led.setCorrection(TypicalLEDStrip);
    led.setBrightness(config.maxBrightness);

    led.clear();
    led.show();

    wifi_connect(WIFI_MODE, WIFI_MAX_CONNECTION_ATTEMPT_INTERVAL);

    udpServer.begin(UDP_PORT);
    wsServer.begin(webServer);

    webServer.begin();
}

void render();

void loop() {
    wifi_check_connection();

    udpServer.handle_incoming_data();
    wsServer.handle_incoming_data();

    if (config.power) {
        render();
    } else {
        led.clear();
        led.show();
    }

    delay(1000 / FRAMES_PER_SECOND);
}

void render() {
    const auto &effectFn = appConfig.colorEffectFn;
    const auto &brightnessFn = appConfig.brightnessEffectFn;
    const auto &palette = (const CRGBPalette16 &) *appConfig.palette;

    led.clear();

    led.setBrightness(config.maxBrightness);
    effectFn(led, palette, config.scale, config.speed);
    brightnessFn(led, config.light);

    led.show();
}