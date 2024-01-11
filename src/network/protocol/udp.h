#pragma once

#include "Arduino.h"
#include <WiFiUdp.h>

#include "base.h"
#include "config.h"

class UdpServer : ServerBase {
    WiFiUDP _udp;
    byte _buffer[UDP_MAX_PACKET_SIZE];

public:
    explicit UdpServer(AppConfig &config);

    void begin(uint16_t port);

    void handle_incoming_data() override;

protected:
    bool handle_incoming_data_impl(unsigned int packetSize);
};

