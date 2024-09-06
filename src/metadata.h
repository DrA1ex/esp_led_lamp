#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <map>

#include <Arduino.h>

#include "config.h"
#include "network/enum.h"
#include "utils/enum.h"

class Application;

MAKE_ENUM(NotificationProperty, uint8_t,
          POWER, 0,
          BRIGHTNESS, 1,
          COLOR, 2,
          PRESET, 3,
          PALETTE, 4,
          NIGHT_MODE_ENABLED, 5,
)

struct PropertyMetadata {
    NotificationProperty property{};
    PacketType packet_type{};

    uint8_t value_offset{};
    uint8_t value_size{};

    const char *mqtt_in_topic = nullptr;
    const char *mqtt_out_topic = nullptr;
};

extern std::map<PacketType, PropertyMetadata> PacketTypeMetadataMap;
extern std::map<NotificationProperty, std::vector<PropertyMetadata>> PropertyMetadataMap;
extern std::map<String, PropertyMetadata> TopicPropertyMetadata;

std::map<NotificationProperty, std::vector<PropertyMetadata>> _build_property_metadata_map(
        std::map<PacketType, PropertyMetadata> &packetMapping);

std::map<String, PropertyMetadata> _build_topic_property_metadata_map(
        std::map<PacketType, PropertyMetadata> &packetMapping);