#include "udp.h"

#include "constants.h"
#include "debug.h"

UdpServer::UdpServer(AppConfig &config) : ServerBase(config) {}

void UdpServer::begin(uint16_t port) {
    _udp.begin(port);

    D_WRITE("UDP server listening on port: ");
    D_PRINT(port);
}

void UdpServer::handle_incoming_data() {
    for (;;) {
        unsigned int packetSize = _udp.parsePacket();
        if (packetSize == 0) return;

        const bool success = handle_incoming_data_impl(packetSize);

        _udp.beginPacket(_udp.remoteIP(), _udp.remotePort());
        _udp.write(success ? "OK" : "ERROR");
        _udp.endPacket();
    }
}

bool UdpServer::handle_incoming_data_impl(unsigned int packetSize) {
    D_WRITE("Received UDP packet, size: ");
    D_PRINT(packetSize);

    _udp.read(_buffer, UDP_MAX_PACKET_SIZE);

    D_WRITE("Packet body: ");
    for (unsigned int i = 0; i < packetSize; ++i) {
        D_WRITE(String(_buffer[i], HEX));
        D_WRITE(" ");
    }
    D_PRINT();

    const auto header_size = sizeof(PacketHeader);
    if (packetSize < header_size) {
        D_WRITE("Wrong packet size. Expected at least: ");
        D_PRINT(header_size);

        return false;
    }

    auto *packet = (PacketHeader *) _buffer;
    if (packet->signature != UDP_PACKET_SIGNATURE) {
        D_WRITE("Wrong packet signature: ");
        D_PRINT(packet->signature);

        return false;
    }

    if (header_size + packet->size != packetSize) {
        D_WRITE("Wrong message length, expected ");
        D_PRINT(header_size + packet->size);

        return false;
    }

    const void *data = _buffer + header_size;
    if (packet->type >= PacketType::DISCOVERY) {
        return process_command(*packet, data);
    } else {
        bool success = update_parameter(*packet, data);
        if (success) app_config().update();

        return success;
    }
}
