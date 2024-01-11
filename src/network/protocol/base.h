#pragma once

#include "config.h"
#include "../data.h"

class ServerBase {
    AppConfig &_app_config;

public:
    explicit ServerBase(AppConfig &config);
    virtual ~ServerBase() = default;

    virtual void handle_incoming_data() = 0;

protected:
    inline AppConfig &app_config() { return _app_config; }

    bool handle_packet_data(const byte *buffer, uint16_t length);

    bool update_parameter(const PacketHeader &header, const void *data);
    bool process_command(const PacketHeader &header, const void *data);
};