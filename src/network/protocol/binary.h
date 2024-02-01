#pragma once

#include <type_traits>

#include "config.h"
#include "network/protocol/type.h"

class BinaryProtocol {
public:
    PacketParsingResponse parse_packet(const uint8_t *buffer, uint8_t length);

    template<typename T, typename = std::enable_if<std::is_enum_v<T> || std::is_integral_v<T>>>
    Response update_parameter_value(T *parameter, const PacketHeader &header, const void *data);

    Response update_string_value(char *str, uint8_t max_size, const PacketHeader &header, const void *data);

    template<uint8_t StrSize>
    Response update_string_list_value(char destination[][StrSize], uint8_t max_count, const PacketHeader &header, const void *data);

    Response update_preset_configs(PresetConfigs *cfg, const PacketHeader &header, const void *data);

    Response update_palette(CRGBPalette16 *palette, const PacketHeader &header, const void *data);

    template<typename T>
    Response serialize(const T &obj);

    template<typename C, typename V>
    Response serialize_fx_config(const FxConfig<FxConfigEntry<C, V>> &config);

    template<typename E>
    constexpr auto to_underlying(E e) noexcept;
};

template<typename E>
constexpr auto BinaryProtocol::to_underlying(E e) noexcept {
    if constexpr (std::is_enum_v<E>) {
        return static_cast<std::underlying_type_t<E>>(e);
    } else {
        return static_cast<E>(e);
    }
}

template<typename T>
Response BinaryProtocol::serialize(const T &obj) {
    return Response{ResponseType::BINARY, {.buffer = {.size = sizeof(obj), .data=(uint8_t *) &obj}}};
}

template<typename C, typename V>
Response BinaryProtocol::serialize_fx_config(const FxConfig<FxConfigEntry<C, V>> &config) {
    const int CONFIG_DATA_MAX_LENGTH = 1024;

    uint8_t buffer[CONFIG_DATA_MAX_LENGTH];
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

template<uint8_t StrSize>
Response BinaryProtocol::update_string_list_value(char (*destination)[StrSize], const uint8_t max_count, const PacketHeader &header,
                                                  const void *data) {
    if (header.size < 2) {
        D_PRINTF("Unable to update string list, bad size. Got %u, expected at least %u\n", header.size, 2);
        return Response::code(ResponseCode::BAD_REQUEST);
    }

    uint8_t dst_index;
    memcpy(&dst_index, data, sizeof(dst_index));
    size_t offset = 1;

    if (dst_index >= max_count) {
        D_PRINTF("Unable to update string list, bad destination offset. Got %u, but limit is %u\n", dst_index, max_count - 1);
        return Response::code(ResponseCode::BAD_REQUEST);
    }

    const char *input = (const char *) data + offset;

    size_t updated_count = 0;
    while (offset < header.size) {
        if (dst_index >= max_count) {
            D_PRINT("Unable to finish update. Received too many values");
            break;
        }

        const size_t length = strnlen(input, header.size - offset);
        if (length > StrSize) D_PRINTF("Value at %u will be truncated. Read size %u, but limit is %u\n", dst_index, length, StrSize);

        memcpy(destination[dst_index], input, min((uint8_t) length, StrSize));
        if (length < StrSize) destination[dst_index][length] = '\0';

        D_PRINTF("Update #%u: %.*s (%u)\n", dst_index, StrSize, destination[dst_index], length);

        dst_index++;
        updated_count++;
        offset += length + 1;
        input += length + 1;
    }

    D_WRITE("Update string list ");
    D_WRITE(to_underlying(header.type));
    D_PRINTF(" (Count: %i)\n", updated_count);

    return Response::ok();
}

template<typename T, typename>
Response BinaryProtocol::update_parameter_value(T *parameter, const PacketHeader &header, const void *data) {
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
