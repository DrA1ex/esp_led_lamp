#include "udp.h"

#include "constants.h"
#include "debug.h"

UdpServer::UdpServer(AppConfig &config) : _appConfig(config) {}

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

    const auto header_size = sizeof(UdpPacketHeader);
    if (packetSize < header_size) {
        D_WRITE("Wrong packet size. Expected at least: ");
        D_PRINT(header_size);

        return false;
    }

    auto *packet = (UdpPacketHeader *) _buffer;
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
    if (packet->type >= UdpPacketType::DISCOVERY) {
        return process_command(*packet, data);
    } else {
        bool success = update_parameter(*packet, data);
        if (success) _appConfig.update();

        return success;
    }
}

template<typename T, typename = std::enable_if<std::is_enum<T>::value || std::is_integral<T>::value>>
bool update_parameter_value(T &parameter, const UdpPacketHeader &header, const void *data) {
    if (header.size != sizeof(T)) {
        D_WRITE("Unable to update value, bad size. Got ");
        D_WRITE(header.size);
        D_WRITE(", expected ");
        D_PRINT(sizeof(T));

        return false;
    }

    parameter = *((T *) data);
    D_WRITE("Update parameter ");
    D_WRITE(header.type);
    D_WRITE(" = ");
    D_PRINT(parameter);

    return true;
}

bool UdpServer::update_parameter(const UdpPacketHeader &header, const void *data) {
    switch (header.type) {
        case SPEED:
            return update_parameter_value(_appConfig.config.speed, header, data);

        case SCALE:
            return update_parameter_value(_appConfig.config.scale, header, data);

        case LIGHT:
            return update_parameter_value(_appConfig.config.light, header, data);

        case MAX_BRIGHTNESS:
            return update_parameter_value(_appConfig.config.maxBrightness, header, data);

        case PALETTE:
            return update_parameter_value(_appConfig.config.palette, header, data);

        case COLOR_EFFECT:
            return update_parameter_value(_appConfig.config.colorEffect, header, data);

        case BRIGHTNESS_EFFECT:
            return update_parameter_value(_appConfig.config.brightnessEffect, header, data);

        default:
            D_WRITE("Unable to update value, bad type: ");
            D_PRINT(header.type);
            return false;
    }
}
bool UdpServer::process_command(const UdpPacketHeader &header, const void *data) {
    return false;
}
