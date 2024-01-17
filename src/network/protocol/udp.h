#pragma once

#include "Arduino.h"
#include <WiFiUdp.h>

#include "base.h"

class UdpServer : ServerBase {
    WiFiUDP _udp;
    byte _buffer[UDP_MAX_PACKET_SIZE];

public:
    explicit UdpServer(Application &app);

    void begin(uint16_t port);

    void handle_incoming_data() override;

protected:
    Response handle_incoming_data_impl(unsigned int packet_size);
};

