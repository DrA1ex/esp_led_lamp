#include "udp.h"

#include "constants.h"
#include "debug.h"

UdpServer::UdpServer(Application &app) : ServerBase(app) {}

void UdpServer::begin(uint16_t port) {
    _udp.begin(port);

    D_WRITE("UDP server listening on port: ");
    D_PRINT(port);
}

void UdpServer::handle_incoming_data() {
    for (;;) {
        unsigned int packetSize = _udp.parsePacket();
        if (packetSize == 0) return;

        auto response = handle_incoming_data_impl(packetSize);

        _udp.beginPacket(_udp.remoteIP(), _udp.remotePort());

        switch (response.type) {
            case ResponseType::CODE:
                _udp.write(response.codeString());
                break;

            case ResponseType::STRING:
                _udp.write(response.body.str);
                break;

            case ResponseType::BINARY:
                _udp.write(response.body.buffer.data, response.body.buffer.size);
                break;
        }

        _udp.endPacket();
    }
}

Response UdpServer::handle_incoming_data_impl(unsigned int packetSize) {
    D_WRITE("Received UDP packet, size: ");
    D_PRINT(packetSize);

    _udp.read(_buffer, UDP_MAX_PACKET_SIZE);

    return handle_packet_data(_buffer, packetSize);
}
