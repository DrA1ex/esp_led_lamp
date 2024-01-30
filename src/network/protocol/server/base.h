#pragma once

#include "config.h"

#include "network/protocol/binary.h"
#include "network/protocol/type.h"

class Application;

class ServerBase {
    Application &_app;
    BinaryProtocol _protocol;

public:
    explicit ServerBase(Application &app);
    virtual ~ServerBase() = default;

    virtual void handle_incoming_data() = 0;

protected:
    inline Application &app() { return _app; }

    Response handle_packet_data(const byte *buffer, uint16_t length);

    Response update_parameter(const PacketHeader &header, const void *data);
    Response calibrate(const PacketHeader &header, const void *data);
    Response process_command(const PacketHeader &header);
    Response process_data_request(const PacketHeader &header);
};