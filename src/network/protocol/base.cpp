#include "base.h"

#include "fx/fx.h"

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
    if (packet->signature != PACKET_SIGNATURE) {
        D_PRINTF("Wrong packet signature: %X\n", packet->signature);

        return Response::code(ResponseCode::BAD_REQUEST);
    }

    if (header_size + packet->size != length) {
        D_PRINTF("Wrong message length, expected: %u\n", header_size + packet->size);

        return Response::code(ResponseCode::BAD_REQUEST);
    }

    const void *data = buffer + header_size;
    if (packet->type >= PacketType::DISCOVERY) {
        auto response = process_command(*packet);
        if (response.isOk() && packet->type >= PacketType::POWER_OFF) app_config().update();

        return response;
    } else if (packet->type >= PacketType::PALETTE_LIST) {
        auto response = process_data_request(*packet);
        if (response.isOk()) D_PRINTF("Data request %u, size: %u \n", (uint8_t) packet->type, response.body.buffer.size);

        return response;
    } else if (packet->type >= PacketType::CALIBRATION_R) {
        auto response = calibrate(*packet, data);
        if (response.isOk()) {
            D_PRINTF("Color correction: %Xu\n", app_config().config.colorCorrection);

            app_config().storage.save();
            app_config().changeState(AppState::CALIBRATION);
        }

        return response;
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
    D_WRITE(to_underlying(header.type));
    D_WRITE(" = ");
    D_PRINT(to_underlying(parameter));

    return Response::ok();
}

Response ServerBase::update_parameter(const PacketHeader &header, const void *data) {
    switch (header.type) {
        case PacketType::SPEED:
            return update_parameter_value(app_config().config.speed, header, data);

        case PacketType::SCALE:
            return update_parameter_value(app_config().config.scale, header, data);

        case PacketType::LIGHT:
            return update_parameter_value(app_config().config.light, header, data);

        case PacketType::MAX_BRIGHTNESS:
            return update_parameter_value(app_config().config.maxBrightness, header, data);

        case PacketType::ECO_LEVEL:
            return update_parameter_value(app_config().config.eco, header, data);

        case PacketType::PALETTE:
            return update_parameter_value(app_config().config.palette, header, data);

        case PacketType::COLOR_EFFECT:
            return update_parameter_value(app_config().config.colorEffect, header, data);

        case PacketType::BRIGHTNESS_EFFECT:
            return update_parameter_value(app_config().config.brightnessEffect, header, data);

        default:
            D_PRINTF("Unable to update value, bad type: %u\n", (uint8_t) header.type);
            return Response::code(ResponseCode::BAD_COMMAND);
    }
}

Response ServerBase::process_command(const PacketHeader &header) {
    bool success = true;

    switch (header.type) {
        case PacketType::POWER_ON:
            app_config().config.power = true;
            break;

        case PacketType::POWER_OFF:
            app_config().config.power = false;
            break;

        case PacketType::RESTART:
            app_config().storage.force_save();
            ESP.restart();
            break;

        case PacketType::DISCOVERY:
            break;

        default:
            D_PRINTF("Unknown command: %u\n", (uint8_t) header.type);
            success = false;
    }

    if (success) D_PRINTF("Command %u\n", (uint8_t) header.type);

    return Response::code(success ? ResponseCode::OK : ResponseCode::BAD_COMMAND);
}

template<typename C, typename V>
Response serialize_fx_config(const FxConfig<FxConfigEntry<C, V>> &config) {
    const int CONFIG_DATA_MAX_LENGTH = 1024;

    static uint8_t buffer[CONFIG_DATA_MAX_LENGTH];
    uint16_t size = 0;

    *(buffer + size) = config.count;
    ++size;

    for (int i = 0; i < config.count; ++i) {
        const auto &entry = config.entries[i];

        *(buffer + size) = (uint8_t) entry.code;
        ++size;

        const auto name_length = strlen(entry.name);
        memcpy(buffer + size, entry.name, name_length);
        size += name_length;

        *(buffer + size) = '\0';
        ++size;
    }

    if (size > CONFIG_DATA_MAX_LENGTH) {
        return Response::code(ResponseCode::INTERNAL_ERROR);
    }

    return Response{ResponseType::BINARY, {.buffer = {.size = size, .data=buffer}}};
}

Response serialize_config(const Config &config) {
    const int CONFIG_DATA_MAX_LENGTH = sizeof(config) + 1;
    static uint8_t buffer[CONFIG_DATA_MAX_LENGTH];

    buffer[0] = sizeof(config);
    memcpy(buffer + 1, &config, sizeof(config));

    return Response{ResponseType::BINARY, {.buffer = {.size = CONFIG_DATA_MAX_LENGTH, .data=buffer}}};
}

Response ServerBase::process_data_request(const PacketHeader &header) {
    switch (header.type) {
        case PacketType::COLOR_EFFECT_LIST:
            return serialize_fx_config(ColorEffects.config());

        case PacketType::BRIGHTNESS_EFFECT_LIST:
            return serialize_fx_config(BrightnessEffects.config());

        case PacketType::PALETTE_LIST:
            return serialize_fx_config(Palettes);

        case PacketType::GET_CONFIG:;
            return serialize_config((Config &) app_config().config);

        default:
            return Response{ResponseType::CODE, {.code = ResponseCode::BAD_COMMAND}};
    }
}

Response ServerBase::calibrate(const PacketHeader &header, const void *data) {
    if (header.size != 1) {
        return Response::code(ResponseCode::BAD_REQUEST);
    }

    switch (header.type) {
        case PacketType::CALIBRATION_B:
            app_config().config.colorCorrection &= ~(uint32_t) 0xff;
            app_config().config.colorCorrection |= *((uint8_t *) data);
            break;

        case PacketType::CALIBRATION_G:
            app_config().config.colorCorrection &= ~(uint32_t) 0xff00;
            app_config().config.colorCorrection |= *((uint8_t *) data) << 8;
            break;

        case PacketType::CALIBRATION_R:
            app_config().config.colorCorrection &= ~(uint32_t) 0xff0000;
            app_config().config.colorCorrection |= *((uint8_t *) data) << 16;
            break;

        default:
            return Response::code(ResponseCode::BAD_COMMAND);
    }

    return Response::ok();
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

        case ResponseCode::INTERNAL_ERROR:
        default:
            return "INTERNAL ERROR";
    }
}