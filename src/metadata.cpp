#include "metadata.h"

#include <stddef.h>

#include "constants.h"

std::map<PacketType, PropertyMetadata> PacketTypeMetadataMap = {
        {
                PacketType::POWER_ON,
                {
                        NotificationProperty::POWER,              PacketType::POWER_ON,
                        offsetof(Config, power),              sizeof(Config::power),
                        MQTT_TOPIC_POWER,      MQTT_OUT_TOPIC_POWER,
                }
        },
        {
                PacketType::POWER_OFF,
                {
                        NotificationProperty::POWER,              PacketType::POWER_OFF,
                        offsetof(Config, power),              sizeof(Config::power),
                        MQTT_TOPIC_POWER,      MQTT_OUT_TOPIC_POWER,
                }
        },
        {
                PacketType::MAX_BRIGHTNESS,
                {
                        NotificationProperty::BRIGHTNESS,         PacketType::MAX_BRIGHTNESS,
                        offsetof(Config, max_brightness),     sizeof(Config::max_brightness),
                        MQTT_TOPIC_BRIGHTNESS, MQTT_OUT_TOPIC_BRIGHTNESS,
                }
        },
        {
                PacketType::PRESET_ID,
                {
                        NotificationProperty::PRESET,             PacketType::PRESET_ID,
                        offsetof(Config, preset_id),          sizeof(Config::preset_id),
                        MQTT_TOPIC_PRESET,     MQTT_OUT_TOPIC_PRESET,
                }
        },
        {
                PacketType::NIGHT_MODE_ENABLED,
                {
                        NotificationProperty::NIGHT_MODE_ENABLED, PacketType::NIGHT_MODE_ENABLED,
                        offsetof(Config, night_mode.enabled), sizeof(Config::night_mode.enabled),
                        MQTT_TOPIC_NIGHT_MODE, MQTT_OUT_TOPIC_NIGHT_MODE,
                }
        },
};

std::map<NotificationProperty, std::vector<PropertyMetadata>> PropertyMetadataMap =
        _build_property_metadata_map(PacketTypeMetadataMap);

std::map<String, PropertyMetadata> TopicPropertyMetadata =
        _build_topic_property_metadata_map(PacketTypeMetadataMap);

std::map<NotificationProperty, std::vector<PropertyMetadata>> _build_property_metadata_map(
        std::map<PacketType, PropertyMetadata> &packetMapping) {
    std::map<NotificationProperty, std::vector<PropertyMetadata>> result;

    for (auto &[packetType, metadata]: packetMapping) {
        std::vector<PropertyMetadata> &prop = result[metadata.property];
        prop.push_back(metadata);
    }

    return result;
}

std::map<String, PropertyMetadata> _build_topic_property_metadata_map(
        std::map<PacketType, PropertyMetadata> &packetMapping) {
    std::map<String, PropertyMetadata> result;

    for (auto &[packetType, metadata]: packetMapping) {
        if (metadata.mqtt_in_topic == nullptr) continue;

        result[metadata.mqtt_in_topic] = metadata;
    }

    return result;
}