#pragma once

#include "Arduino.h"
#include <WiFiUdp.h>

#include "config.h"

enum UdpPacketType : uint8_t {
    SPEED = 0,
    SCALE = 1,
    LIGHT = 2,

    MAX_BRIGHTNESS = 50,

    PALETTE = 100,
    COLOR_EFFECT = 101,
    BRIGHTNESS_EFFECT = 102,

    DISCOVERY = 250,
    POWER_OFF = 251,
    POWER_ON = 252,
    RESTART = 253,
};

struct __attribute__ ((packed))  UdpPacketHeader {
    uint16_t signature;
    UdpPacketType type;
    uint8_t size;
};

class UdpServer {
    WiFiUDP _udp;
    byte _buffer[UDP_MAX_PACKET_SIZE];

    AppConfig &_appConfig;

public:
    explicit UdpServer(AppConfig &config);

    void begin(uint16_t port);

    void handle_incoming_data();

protected:
    bool handle_incoming_data_impl(unsigned int packetSize);
    bool update_parameter(const UdpPacketHeader &header, const void *data);
    bool process_command(const UdpPacketHeader &header, const void *data);
};

