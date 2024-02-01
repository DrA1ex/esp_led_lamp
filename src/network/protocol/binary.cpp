#include "binary.h"

#include <string>

#include "debug.h"
#include "fx/fx.h"

PacketParsingResponse BinaryProtocol::parse_packet(const uint8_t *buffer, uint8_t length) {
    D_WRITE("Packet body: ");
    for (unsigned int i = 0; i < length; ++i) {
        D_PRINTF("%02X ", buffer[i]);
    }
    D_PRINT();

    const auto header_size = sizeof(PacketHeader);
    if (length < header_size) {
        D_PRINTF("Wrong packet size. Expected at least: %u\n", header_size);

        return PacketParsingResponse::fail(Response::code(ResponseCode::PACKET_LENGTH_EXCEEDED));
    }

    auto *packet = (PacketHeader *) buffer;
    if (packet->signature != PACKET_SIGNATURE) {
        D_PRINTF("Wrong packet signature: %X\n", packet->signature);

        return PacketParsingResponse::fail(Response::code(ResponseCode::BAD_REQUEST));
    }

    if (header_size + packet->size != length) {
        D_PRINTF("Wrong message length, expected: %u\n", header_size + packet->size);

        return PacketParsingResponse::fail(Response::code(ResponseCode::BAD_REQUEST));
    }

    const void *data = buffer + header_size;
    return PacketParsingResponse::ok({packet, data});
}

Response BinaryProtocol::update_string_value(char *str, uint8_t max_size, const PacketHeader &header, const void *data) {
    if (header.size > max_size) {
        D_PRINTF("Unable to update value, data too long. Got %u, but limit is %u\n", header.size, max_size);
        return Response::code(ResponseCode::BAD_REQUEST);
    }

    memcpy(str, data, header.size);
    if (header.size < max_size) str[header.size] = '\0';

    D_WRITE("Update parameter ");
    D_WRITE(to_underlying(header.type));
    D_PRINTF(" = %.*s\n", max_size, str);

    return Response::ok();
}

Response BinaryProtocol::update_preset_configs(PresetConfigs *cfg, const PacketHeader &header, const void *data) {
    if (header.size > 0 && header.size % sizeof(cfg->presets[0])) {
        D_PRINTF("Unable to update value, invalid size. Got %u, but size must be multiple of %u\n", header.size, sizeof(cfg->presets[0]));

        return Response::code(ResponseCode::BAD_REQUEST);
    }


    memcpy(cfg->presets, data, header.size);

    const uint8_t count = header.size / sizeof(cfg->presets[0]);
    for (uint8_t i = 0; i < count; ++i) {
        auto &preset = cfg->presets[i];

        if ((uint8_t) preset.brightness_effect >= BrightnessEffects.config().count) {
            preset.brightness_effect = (BrightnessEffectEnum) 0;
        }

        if ((uint8_t) preset.color_effect >= ColorEffects.config().count) {
            preset.color_effect = (ColorEffectEnum) 0;
        }

        if ((uint8_t) preset.palette >= Palettes.count) {
            preset.palette = (PaletteEnum) 0;
        }
    }

    D_PRINTF("Bulk update preset configs (Count: %u)\n", count);

    return Response::ok();
}

Response BinaryProtocol::update_palette(CustomPaletteConfig *palette, const PacketHeader &header, const void *data) {
    const auto dst_entry_size = sizeof(palette->colors[0]);
    const auto count = sizeof(palette->colors) / dst_entry_size;

    const auto src_entry_size = sizeof(CRGB);
    const auto expected_size = count * src_entry_size;

    if (header.size != expected_size) {
        D_PRINTF("Unable to update palette, invalid size. Got %u, expected %u\n", header.size, expected_size);
        return Response::code(ResponseCode::BAD_REQUEST);
    }


    uint8_t color[dst_entry_size] = {0};
    for (size_t i = 0; i < count; ++i) {
        // Little Endian, 0x00a1b2c3 stored as [c3 b2 a1 00], i.e. [BB GG RR 00]
        color[0] = *((uint8_t *) data + i * src_entry_size + 2);
        color[1] = *((uint8_t *) data + i * src_entry_size + 1);
        color[2] = *((uint8_t *) data + i * src_entry_size + 0);

        memcpy(&palette->colors[i], color, dst_entry_size);
    }

    palette->updated = true;

    D_PRINT("Update palette");

    return Response::ok();
}
