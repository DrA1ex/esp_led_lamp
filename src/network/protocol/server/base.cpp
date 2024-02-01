#include "base.h"

#include "application.h"
#include "fx/fx.h"
#include "fx/palette.h"

ServerBase::ServerBase(Application &config) : _app(config), _protocol() {}

Response ServerBase::handle_packet_data(const uint8_t *buffer, uint16_t length) {
    const auto parseResponse = _protocol.parse_packet(buffer, length);
    if (!parseResponse.success) return parseResponse.response;

    const auto [header, data] = parseResponse.packet;

    if (header->type >= PacketType::DISCOVERY) {
        auto response = process_command(*header);
        if (response.is_ok() && header->type >= PacketType::POWER_OFF) app().update();

        return response;
    } else if (header->type >= PacketType::PALETTE_LIST) {
        auto response = process_data_request(*header);
        if (response.is_ok()) D_PRINTF("Data request %u, size: %u \n", (uint8_t) header->type, response.body.buffer.size);

        return response;
    } else if (header->type >= PacketType::CALIBRATION_R) {
        auto response = calibrate(*header, data);
        if (response.is_ok()) {
            D_PRINTF("Color correction: %Xu\n", app().config.color_correction);

            app().config_storage.save();
        }

        return response;
    } else {
        auto response = update_parameter(*header, data);
        if (response.is_ok()) {
            if (header->type >= PacketType::NIGHT_MODE_ENABLED && header->type <= PacketType::NIGHT_MODE_INTERVAL) {
                app().night_mode_manager.reset();
            }

            if (header->type == PacketType::UPDATE_CUSTOM_PALETTE) {
                app().custom_palette_storage.save();
            }

            app().update();
        }

        return response;
    }
}

Response ServerBase::update_parameter(const PacketHeader &header, const void *data) {
    switch (header.type) {
        case PacketType::SPEED:
            return _protocol.update_parameter_value(&app().preset().speed, header, data);

        case PacketType::SCALE:
            return _protocol.update_parameter_value(&app().preset().scale, header, data);

        case PacketType::LIGHT:
            return _protocol.update_parameter_value(&app().preset().light, header, data);

        case PacketType::MAX_BRIGHTNESS:
            return _protocol.update_parameter_value(&app().config.max_brightness, header, data);

        case PacketType::ECO_LEVEL:
            return _protocol.update_parameter_value(&app().config.eco, header, data);

        case PacketType::GAMMA:
            return _protocol.update_parameter_value(&app().config.gamma, header, data);

        case PacketType::PALETTE:
            return _protocol.update_parameter_value(&app().preset().palette, header, data);

        case PacketType::COLOR_EFFECT:
            return _protocol.update_parameter_value(&app().preset().color_effect, header, data);

        case PacketType::BRIGHTNESS_EFFECT:
            return _protocol.update_parameter_value(&app().preset().brightness_effect, header, data);

        case PacketType::NIGHT_MODE_ENABLED:
            return _protocol.update_parameter_value(&app().config.night_mode.enabled, header, data);

        case PacketType::NIGHT_MODE_BRIGHTNESS:
            return _protocol.update_parameter_value(&app().config.night_mode.brightness, header, data);

        case PacketType::NIGHT_MODE_ECO:
            return _protocol.update_parameter_value(&app().config.night_mode.eco, header, data);

        case PacketType::NIGHT_MODE_START:
            return _protocol.update_parameter_value(&app().config.night_mode.start_time, header, data);

        case PacketType::NIGHT_MODE_END:
            return _protocol.update_parameter_value(&app().config.night_mode.end_time, header, data);

        case PacketType::NIGHT_MODE_INTERVAL:
            return _protocol.update_parameter_value(&app().config.night_mode.switch_interval, header, data);

        case PacketType::PRESET_ID:
            return _protocol.update_parameter_value(&app().config.preset_id, header, data);

        case PacketType::PRESET_NAME:
            return _protocol.update_string_value(app().preset_names.names[app().config.preset_id], PRESET_NAME_MAX_SIZE, header, data);

        case PacketType::UPDATE_PRESET_NAMES:
            return _protocol.update_string_list_value(app().preset_names.names, PRESET_MAX_COUNT, header, data);

        case PacketType::UPDATE_PRESET_CONFIGS:
            return _protocol.update_preset_configs(&app().preset_configs, header, data);

        case PacketType::UPDATE_CUSTOM_PALETTE:
            return _protocol.update_palette(&app().custom_palette_config, header, data);

        default:
            D_PRINTF("Unable to update value, bad type: %u\n", (uint8_t) header.type);
            return Response::code(ResponseCode::BAD_COMMAND);
    }
}

Response ServerBase::process_command(const PacketHeader &header) {
    bool success = true;

    switch (header.type) {
        case PacketType::POWER_ON:
            app().set_power(true);
            break;

        case PacketType::POWER_OFF:
            app().set_power(false);
            break;

        case PacketType::RESTART:
            app().restart();
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
    const int CONFIG_DATA_MAX_LENGTH = 1536;

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

        case PacketType::PRESET_NAMES_LIST:
            return serialize(app().preset_names);

        case PacketType::PRESET_CONFIG_LIST:
            return serialize(app().preset_configs);

        case PacketType::GET_PALETTE:
            return serialize(app().current_palette.entries);

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

    app().change_state(AppState::CALIBRATION);
    return Response::ok();
}
