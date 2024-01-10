#pragma once

#include "Arduino.h"
#include <WiFiUdp.h>

#include "config.h"
#include "constants.h"

enum UdpPacketType {
    SPEED,
    SCALE,
    LIGHT,

    PALETTE,
    COLOR_EFFECT,
    BRIGHTNESS_EFFECT,
};

struct UdpPacket {
    uint16_t signature;
    UdpPacketType type;
    uint16_t data;
};

class UdpServer {
    WiFiUDP _udp;
    byte _buffer[UDP_MAX_PACKET_SIZE];

    AppConfig &_appConfig;

public:
    UdpServer(AppConfig &config);

    void begin(uint16_t port);

    void handle_incoming_data();

protected:
    bool update_parameter(UdpPacketType type, uint16_t value);
};

