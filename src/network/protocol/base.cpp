#include "base.h"

ServerBase::ServerBase(AppConfig &config) : _app_config(config) {}

template<typename T, typename = std::enable_if<std::is_enum<T>::value || std::is_integral<T>::value>>
bool update_parameter_value(T &parameter, const PacketHeader &header, const void *data) {
    if (header.size != sizeof(T)) {
        D_PRINTF("Unable to update value, bad size. Got %u, expected %u\n", header.size, sizeof(T));

        return false;
    }

    parameter = *((T *) data);
    D_WRITE("Update parameter ");
    D_WRITE(header.type);
    D_WRITE(" = ");
    D_PRINT(parameter);

    return true;
}

bool ServerBase::update_parameter(const PacketHeader &header, const void *data) {
    switch (header.type) {
        case SPEED:
            return update_parameter_value(app_config().config.speed, header, data);

        case SCALE:
            return update_parameter_value(app_config().config.scale, header, data);

        case LIGHT:
            return update_parameter_value(app_config().config.light, header, data);

        case MAX_BRIGHTNESS:
            return update_parameter_value(app_config().config.maxBrightness, header, data);

        case PALETTE:
            return update_parameter_value(app_config().config.palette, header, data);

        case COLOR_EFFECT:
            return update_parameter_value(app_config().config.colorEffect, header, data);

        case BRIGHTNESS_EFFECT:
            return update_parameter_value(app_config().config.brightnessEffect, header, data);

        default:
            D_PRINTF("Unable to update value, bad type: %u\n", header.type);
            return false;
    }
}

bool ServerBase::process_command(const PacketHeader &header, const void *data) {
    switch (header.type) {
        case POWER_ON:
            app_config().config.power = true;
            return true;

        case POWER_OFF:
            app_config().config.power = false;
            return true;

        case RESTART:
            ESP.restart();
            return true;

        default:
            D_PRINTF("Unknown command: %u\n", header.type);
            return false;
    }
}

bool ServerBase::handle_packet_data(const byte *buffer, uint16_t length) {
    D_WRITE("Packet body: ");
    for (unsigned int i = 0; i < length; ++i) {
        D_WRITE(String(buffer[i], HEX));
        D_WRITE(" ");
    }
    D_PRINT();

    const auto header_size = sizeof(PacketHeader);
    if (length < header_size) {
        D_PRINTF("Wrong packet size. Expected at least: %u\n", header_size);

        return false;
    }

    auto *packet = (PacketHeader *) buffer;
    if (packet->signature != UDP_PACKET_SIGNATURE) {
        D_PRINTF("Wrong packet signature: %X\n", packet->signature);

        return false;
    }

    if (header_size + packet->size != length) {
        D_PRINTF("Wrong message length, expected: %u\n", header_size + packet->size);

        return false;
    }

    const void *data = buffer + header_size;
    if (packet->type >= PacketType::DISCOVERY) {
        bool success = process_command(*packet, data);
        if (success) D_PRINTF("Command %u\n", packet->type);

        return success;
    } else {
        bool success = update_parameter(*packet, data);
        if (success) app_config().update();

        return success;
    }
}
