#include "base.h"

#include "application.h"

#include "fx/fx.h"

ServerBase::ServerBase(Application &config) : _app(config) {}

Response ServerBase::handle_packet_data(const byte *buffer, uint16_t length) {
    D_WRITE("Packet body: ");
    for (unsigned int i = 0; i < length; ++i) {
        D_PRINTF("%02X ", buffer[i]);
    }
    D_PRINT();

    const auto header_size = sizeof(PacketHeader);
    if (length < header_size) {
        D_PRINTF("Wrong packet size. Expected at least: %u\n", header_size);

        return Response::code(ResponseCode::PACKET_LENGTH_EXCEEDED);
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
        if (response.is_ok() && packet->type >= PacketType::POWER_OFF) app().update();

        return response;
    } else if (packet->type >= PacketType::PALETTE_LIST) {
        auto response = process_data_request(*packet);
        if (response.is_ok()) D_PRINTF("Data request %u, size: %u \n", (uint8_t) packet->type, response.body.buffer.size);

        return response;
    } else if (packet->type >= PacketType::CALIBRATION_R) {
        auto response = calibrate(*packet, data);
        if (response.is_ok()) {
            D_PRINTF("Color correction: %Xu\n", app().config.color_correction);

            app().config_storage.save();
            app().change_state(AppState::CALIBRATION);
        }

        return response;
    } else {
        auto response = update_parameter(*packet, data);
        if (response.is_ok()) {
            if (packet->type >= PacketType::NIGHT_MODE_ENABLED && packet->type <= PacketType::NIGHT_MODE_INTERVAL) {
                app().night_mode_manager.reset();
            }

            app().update();
        }

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
Response update_parameter_value(T *parameter, const PacketHeader &header, const void *data) {
    if (header.size != sizeof(T)) {
        D_PRINTF("Unable to update value, bad size. Got %u, expected %u\n", header.size, sizeof(T));

        return Response::code(ResponseCode::BAD_REQUEST);
    }

    memcpy(parameter, data, sizeof(T));

    D_WRITE("Update parameter ");
    D_WRITE(to_underlying(header.type));
    D_WRITE(" = ");
    D_PRINT(to_underlying(*(T *) data));

    return Response::ok();
}

Response update_string_value(char *str, uint8_t max_size, const PacketHeader &header, const void *data) {
    if (header.size > max_size) {
        D_PRINTF("Unable to update value, data too long. Got %u, but limit is %u\n", header.size, max_size);

        return Response::code(ResponseCode::BAD_REQUEST);
    }

    memcpy(str, data, header.size);
    if (header.size < max_size) str[header.size] = '\0';

    D_WRITE("Update parameter ");
    D_WRITE(to_underlying(header.type));
    D_WRITE(" = ");
    D_PRINT(str);

    return Response::ok();
}

Response ServerBase::update_parameter(const PacketHeader &header, const void *data) {
    switch (header.type) {
        case PacketType::SPEED:
            return update_parameter_value(&app().preset().speed, header, data);

        case PacketType::SCALE:
            return update_parameter_value(&app().preset().scale, header, data);

        case PacketType::LIGHT:
            return update_parameter_value(&app().preset().light, header, data);

        case PacketType::MAX_BRIGHTNESS:
            return update_parameter_value(&app().config.max_brightness, header, data);

        case PacketType::ECO_LEVEL:
            return update_parameter_value(&app().config.eco, header, data);

        case PacketType::PALETTE:
            return update_parameter_value(&app().preset().palette, header, data);

        case PacketType::COLOR_EFFECT:
            return update_parameter_value(&app().preset().color_effect, header, data);

        case PacketType::BRIGHTNESS_EFFECT:
            return update_parameter_value(&app().preset().brightness_effect, header, data);

        case PacketType::NIGHT_MODE_ENABLED:
            return update_parameter_value(&app().config.night_mode.enabled, header, data);

        case PacketType::NIGHT_MODE_BRIGHTNESS:
            return update_parameter_value(&app().config.night_mode.brightness, header, data);

        case PacketType::NIGHT_MODE_ECO:
            return update_parameter_value(&app().config.night_mode.eco, header, data);

        case PacketType::NIGHT_MODE_START:
            return update_parameter_value(&app().config.night_mode.start_time, header, data);

        case PacketType::NIGHT_MODE_END:
            return update_parameter_value(&app().config.night_mode.end_time, header, data);

        case PacketType::NIGHT_MODE_INTERVAL:
            return update_parameter_value(&app().config.night_mode.switch_interval, header, data);

        case PacketType::PRESET_ID:
            return update_parameter_value(&app().config.preset_id, header, data);

        case PacketType::PRESET_NAME:
            return update_string_value(app().preset_names.names[app().config.preset_id], PRESET_NAME_MAX_SIZE, header, data);

        default:
            D_PRINTF("Unable to update value, bad type: %u\n", (uint8_t) header.type);
            return Response::code(ResponseCode::BAD_COMMAND);
    }
}

Response ServerBase::process_command(const PacketHeader &header) {
    bool success = true;

    switch (header.type) {
        case PacketType::POWER_ON:
            app().config.power = true;
            break;

        case PacketType::POWER_OFF:
            app().config.power = false;
            break;

        case PacketType::RESTART:
            if (app().config_storage.is_pending_commit()) app().config_storage.force_save();
            if (app().preset_names_storage.is_pending_commit()) app().preset_names_storage.force_save();
            if (app().preset_configs_storage.is_pending_commit()) app().preset_configs_storage.force_save();

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

template<typename T>
Response serialize(const T &obj) {
    return Response{ResponseType::BINARY, {.buffer = {.size = sizeof(obj), .data=(uint8_t *) &obj}}};
}

Response ServerBase::process_data_request(const PacketHeader &header) {
    switch (header.type) {
        case PacketType::COLOR_EFFECT_LIST:
            return serialize_fx_config(ColorEffects.config());

        case PacketType::BRIGHTNESS_EFFECT_LIST:
            return serialize_fx_config(BrightnessEffects.config());

        case PacketType::PALETTE_LIST:
            return serialize_fx_config(Palettes);

        case PacketType::GET_CONFIG:
            return serialize(app().config);

        case PacketType::GET_PRESET_CONFIG:
            return serialize(app().preset());

        case PacketType::PRESET_LIST:
            return serialize(app().preset_names);

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
            app().config.color_correction &= ~(uint32_t) 0xff;
            app().config.color_correction |= *((uint8_t *) data);
            break;

        case PacketType::CALIBRATION_G:
            app().config.color_correction &= ~(uint32_t) 0xff00;
            app().config.color_correction |= *((uint8_t *) data) << 8;
            break;

        case PacketType::CALIBRATION_R:
            app().config.color_correction &= ~(uint32_t) 0xff0000;
            app().config.color_correction |= *((uint8_t *) data) << 16;
            break;

        default:
            return Response::code(ResponseCode::BAD_COMMAND);
    }

    return Response::ok();
}

const char *Response::code_string() {
    if (type != ResponseType::CODE) return nullptr;

    switch (body.code) {
        case ResponseCode::OK:
            return "OK";

        case ResponseCode::BAD_REQUEST:
            return "BAD REQUEST";

        case ResponseCode::BAD_COMMAND:
            return "BAD COMMAND";

        case ResponseCode::TOO_MANY_REQUEST:
            return "TOO MANY REQUESTS";

        case ResponseCode::PACKET_LENGTH_EXCEEDED:
            return "PACKET LENGTH EXCEEDED";

        case ResponseCode::INTERNAL_ERROR:
        default:
            return "INTERNAL ERROR";
    }
}