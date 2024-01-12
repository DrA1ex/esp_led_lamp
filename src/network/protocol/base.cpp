#include "base.h"

ServerBase::ServerBase(AppConfig &config) : _app_config(config) {}

Response ServerBase::handle_packet_data(const byte *buffer, uint16_t length) {
    D_WRITE("Packet body: ");
    for (unsigned int i = 0; i < length; ++i) {
        D_WRITE(String(buffer[i], HEX));
        D_WRITE(" ");
    }
    D_PRINT();

    const auto header_size = sizeof(PacketHeader);
    if (length < header_size) {
        D_PRINTF("Wrong packet size. Expected at least: %u\n", header_size);

        return Response::code(ResponseCode::BAD_REQUEST);
    }

    auto *packet = (PacketHeader *) buffer;
    if (packet->signature != UDP_PACKET_SIGNATURE) {
        D_PRINTF("Wrong packet signature: %X\n", packet->signature);

        return Response::code(ResponseCode::BAD_REQUEST);
    }

    if (header_size + packet->size != length) {
        D_PRINTF("Wrong message length, expected: %u\n", header_size + packet->size);

        return Response::code(ResponseCode::BAD_REQUEST);
    }

    const void *data = buffer + header_size;
    if (packet->type >= PacketType::DISCOVERY) {
        return process_command(*packet);
    } else if (packet->type >= PacketType::PALETTE_LIST) {
        return process_data_request(*packet);
    } else {
        auto response = update_parameter(*packet, data);
        if (response.isOk()) app_config().update();

        return response;
    }
}

template<typename E>
constexpr auto to_underlying(E e) noexcept {
    if constexpr (std::is_enum_v<E>) {
        return static_cast<std::underlying_type_t<E>>(e);
    } else {
        return static_cast<E>(e);
    }
}

template<typename T, typename = std::enable_if<std::is_enum<T>::value || std::is_integral<T>::value>>
Response update_parameter_value(T &parameter, const PacketHeader &header, const void *data) {
    if (header.size != sizeof(T)) {
        D_PRINTF("Unable to update value, bad size. Got %u, expected %u\n", header.size, sizeof(T));

        return Response::code(ResponseCode::BAD_REQUEST);
    }

    parameter = *((T *) data);
    D_WRITE("Update parameter ");
    D_WRITE(header.type);
    D_WRITE(" = ");
    D_PRINT(to_underlying(parameter));

    return Response::ok();
}

Response ServerBase::update_parameter(const PacketHeader &header, const void *data) {
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
            return Response::code(ResponseCode::BAD_COMMAND);
    }
}

Response ServerBase::process_command(const PacketHeader &header) {
    bool success = true;

    switch (header.type) {
        case POWER_ON:
            app_config().config.power = true;
            break;

        case POWER_OFF:
            app_config().config.power = false;
            break;

        case RESTART:
            ESP.restart();
            break;

        default:
            D_PRINTF("Unknown command: %u\n", header.type);
            success = false;
    }

    if (success) D_PRINTF("Command %u\n", header.type);

    return Response::code(success ? ResponseCode::OK : ResponseCode::BAD_COMMAND);
}

Response ServerBase::process_data_request(const PacketHeader &header) {
    return Response{ResponseType::CODE, {.code = ResponseCode::BAD_REQUEST}};
}

const char *Response::codeString() {
    if (type != ResponseType::CODE) return nullptr;

    switch (body.code) {
        case ResponseCode::OK:
            return "OK";

        case ResponseCode::BAD_REQUEST:
            return "BAD REQUEST";

        case ResponseCode::BAD_COMMAND:
            return "BAD COMMAND";

        default:
            return "INTERNAL ERROR";
    }
}
