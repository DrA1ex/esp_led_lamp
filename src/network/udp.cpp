#include "udp.h"

#include "debug.h"

UdpServer::UdpServer(AppConfig &config) : _appConfig(config) {}

void UdpServer::begin(uint16_t port) {
    _udp.begin(port);

    D_WRITE("UDP server listening on port: ");
    D_PRINT(port);
}

void UdpServer::handle_incoming_data() {
    int packetSize = _udp.parsePacket();
    if (packetSize) {
        D_WRITE("Received UDP packet, size: ");
        D_PRINT(packetSize);

        _udp.read(_buffer, UDP_MAX_PACKET_SIZE);

        UdpPacket *packet = (UdpPacket *) _buffer;
        bool success = false;

        if (packet->signature == UDP_PACKET_SIGNATURE) {
            success = update_parameter(packet->type, packet->data);
        } else {
            D_WRITE("Wrong packet signature: ");
            D_PRINT(packet->signature);
        }

        if (success) {
            _udp.beginPacket(_udp.remoteIP(), _udp.remotePort());
            _udp.write("OK");
            _udp.endPacket();
        }
    }
}

bool UdpServer::update_parameter(UdpPacketType type, uint16_t value) {
    bool success = true;

    switch (type) {
        case SPEED:
            _appConfig.config.speed = (byte) value;
            break;

        case SCALE:
            _appConfig.config.scale = (byte) value;
            break;

        case LIGHT:
            _appConfig.config.light = (byte) value;
            break;

        case PALETTE:
            _appConfig.config.palette = (PaletteEnum) value;
            break;

        case COLOR_EFFECT:
            _appConfig.config.colorEffect = (ColorEffectEnum) value;
            break;

        case BRIGHTNESS_EFFECT:
            _appConfig.config.brightnessEffect = (BrightnessEffectEnum) value;
            break;

        default:
            success = false;
    }

    if (success) {
        D_WRITE("Update parameter: ");
        D_WRITE(type);
        D_WRITE(" = ");
        D_PRINT(value);
    } else {
        D_WRITE("Unsupported type: ");
        D_PRINT(type);
    }

    return success;
}
