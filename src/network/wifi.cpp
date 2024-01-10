#include "wifi.h"

#include <ESP8266WiFi.h>

#include "constants.h"
#include "debug.h"

void wifi_connect(byte mode, unsigned long connection_interval) {
    WiFi.disconnect();

    if (mode == 0u) {
        WiFi.mode(WiFiMode::WIFI_AP);

        String chip_id = String(ESP.getChipId() & 0xffff, HEX);
        chip_id.toUpperCase();

        String ssid = String(WIFI_SSID) + "_" + chip_id;
        WiFi.softAP(ssid, WIFI_PASSWORD);

        D_WRITE("Access point created: ");
        D_PRINT(ssid);

        D_WRITE("IP: ");
        D_PRINT(WiFi.softAPIP());
    } else if (mode == 1u) {
        WiFi.mode(WiFiMode::WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

        D_PRINT("Connecting to Wi-Fi...");

        unsigned long begin_time = millis();
        while (!WiFi.isConnected()) {
            D_WRITE(".");
            delay(500);

            if (connection_interval > 0 && millis() - begin_time > connection_interval) {
                D_PRINT();
                D_PRINT("Wi-Fi connection interval exceeded. Switch to AP mode.");

                wifi_connect(0);
                return;
            }
        }

        D_PRINT();

        D_WRITE("Wi-Fi connected! IP address: ");
        D_PRINT(WiFi.localIP());
    }
}

void wifi_check_connection() {
    static unsigned long last_check = 0;
    if (millis() - last_check < WIFI_CONNECTION_CHECK_INTERVAL) return;

    last_check = millis();
    if (WiFi.getMode() == WiFiMode::WIFI_STA && !WiFi.isConnected()) {
        D_PRINT("Wi-Fi connection lost");
        wifi_connect(WIFI_MODE);
    }
}