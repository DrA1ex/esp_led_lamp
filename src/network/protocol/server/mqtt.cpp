#include "mqtt.h"

MqttServer::MqttServer(Application &app) : _app(app) {}

void MqttServer::begin() {
    if (_state != MqttServerState::UNINITIALIZED) return;

    using namespace std::placeholders;

    _mqttClient.onConnect(std::bind(&MqttServer::_on_connect, this, _1));
    _mqttClient.onDisconnect(std::bind(&MqttServer::_on_disconnect, this, _1));
    _mqttClient.onMessage(std::bind(&MqttServer::_on_message, this, _1, _2, _3, _4, _5, _6));

    _mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    _mqttClient.setCredentials(MQTT_USER, MQTT_PASSWORD);

    _app.event_property_changed.subscribe(this, [this](auto, auto prop, auto) {
        _process_notification(prop);
    });

    _connect();
}


void MqttServer::_connect() {
    _change_state(MqttServerState::CONNECTING);

    _last_connection_attempt_time = millis();
    _mqttClient.connect();
}


void MqttServer::_change_state(MqttServerState state) {
    _state = state;
    _state_change_time = millis();
}


void MqttServer::handle_connection() {
    if (_state == MqttServerState::DISCONNECTED && (millis() - _last_connection_attempt_time) > MQTT_RECONNECT_TIMEOUT) {
        D_PRINT("MQTT Reconnecting...");

        _connect();
    }

    if (_state == MqttServerState::CONNECTING && !_mqttClient.connected() && (millis() - _state_change_time) > MQTT_CONNECTION_TIMEOUT) {
        D_PRINT("MQTT Connection timeout");

        _change_state(MqttServerState::DISCONNECTED);
        _mqttClient.disconnect(true);
    }
}


void MqttServer::_on_connect(bool) {
    D_PRINT("MQTT Connected");

    for (const auto &[topic, _]: TopicPropertyMetadata) {
        _subscribe(topic.c_str(), 1);
    }

    _subscribe(MQTT_TOPIC_COLOR, 1);

    _last_connection_attempt_time = millis();
    _change_state(MqttServerState::CONNECTED);
}


void MqttServer::_on_disconnect(AsyncMqttClientDisconnectReason reason) {
    D_PRINTF("MQTT Disconnected. Reason %u\n", (uint8_t) reason);

    _change_state(MqttServerState::DISCONNECTED);
}


void MqttServer::_on_message(
    char *topic, char *payload, AsyncMqttClientMessageProperties properties,
    size_t len, size_t index, size_t total) {
    D_PRINTF("MQTT Received: %s: \"%.*s\"\n", topic, len, payload);

    String topic_str(topic);
    String payload_str{};
    payload_str.concat(payload, len);

    _transform_topic_payload(topic_str, payload_str);

    //TODO: Refactor
    if (topic_str == MQTT_TOPIC_SPEED) {
        app().preset().speed = payload_str.toInt();
        _app.event_property_changed.publish(this, NotificationProperty::SPEED);
    } else if (topic_str == MQTT_TOPIC_SCALE) {
        app().preset().scale = payload_str.toInt();
        _app.event_property_changed.publish(this, NotificationProperty::SCALE);
    } else if (topic_str == MQTT_TOPIC_LIGHT) {
        app().preset().light = payload_str.toInt();
        _app.event_property_changed.publish(this, NotificationProperty::LIGHT);
    } else if (topic_str == MQTT_TOPIC_PALETTE) {
        app().preset().palette = (PaletteEnum) payload_str.toInt();
        _app.event_property_changed.publish(this, NotificationProperty::PALETTE);
    } else if (topic_str == MQTT_TOPIC_COLOR) {
        app().change_color(payload_str.toInt());
        _app.event_property_changed.publish(this, NotificationProperty::COLOR);
    } else {
        _process_message(topic_str, payload_str);
    }
}


void MqttServer::_subscribe(const char *topic, uint8_t qos) {
    _mqttClient.subscribe(topic, qos);
    D_PRINTF("MQTT Subscribe: \"%s\"\n", topic);
}


void MqttServer::_publish(const char *topic, uint8_t qos, const char *payload, size_t length) {
    if (_state != MqttServerState::CONNECTED) {
        D_PRINTF("MQTT Not connected. Skip message to %s\n", topic);
        return;
    };

    _mqttClient.publish(topic, qos, true, payload, length);

    D_PRINTF("MQTT Publish: %s: \"%.*s\"\n", topic, length, payload);
}


void MqttServer::_process_message(const String &topic, const String &payload) {
    auto iter_meta = TopicPropertyMetadata.find(topic);
    if (iter_meta == TopicPropertyMetadata.end()) {
        D_PRINTF("MQTT: Message in unsupported topic: %s\n", topic.c_str());
        return;
    }

    const auto meta = iter_meta->second;
    switch (meta.value_size) {
        case 8:
            _set_value(meta, (uint64_t) payload.toInt());
            break;

        case 4:
            _set_value(meta, (uint32_t) payload.toInt());
            break;

        case 2:
            _set_value(meta, (uint16_t) payload.toInt());
            break;

        case 1:
            _set_value(meta, (uint8_t) payload.toInt());
            break;

        default:
            D_PRINTF("MQTT: Unsupported value size: %u. topic: %s\n", meta.value_size, topic.c_str());
            return;
    }

    _after_message_process(meta);
}


void MqttServer::_after_message_process(const PropertyMetadata &meta) {
    _app.event_property_changed.publish(this, meta.property);
}


void MqttServer::_process_notification(NotificationProperty prop) {
    if (prop == NotificationProperty::SPEED) {
        String str(_app.preset().speed);
        _publish(MQTT_OUT_TOPIC_SPEED, 1, str.c_str(), str.length());
    } else if (prop == NotificationProperty::SCALE) {
        String str(_app.preset().scale);
        _publish(MQTT_OUT_TOPIC_SCALE, 1, str.c_str(), str.length());
    } else if (prop == NotificationProperty::LIGHT) {
        String str(_app.preset().light);
        _publish(MQTT_OUT_TOPIC_LIGHT, 1, str.c_str(), str.length());
    } else if (prop == NotificationProperty::COLOR) {
        String str(_app.current_color());
        _publish(MQTT_OUT_TOPIC_COLOR, 1, str.c_str(), str.length());
        return;
    } else if (prop == NotificationProperty::PALETTE) {
        String str((int) _app.preset().palette);
        _publish(MQTT_OUT_TOPIC_PALETTE, 1, str.c_str(), str.length());
        return;
    }

    auto iter_meta = PropertyMetadataMap.find(prop);
    if (iter_meta == PropertyMetadataMap.end()) return;

    const auto meta = iter_meta->second[0];
    if (meta.mqtt_out_topic == nullptr) return;

    D_PRINTF("MQTT: Processing notification for %s\n", __debug_enum_str(prop));

    switch (meta.value_size) {
        case 8:
            _notify_value_changed<uint64_t>(meta);
            break;

        case 4:
            _notify_value_changed<uint32_t>(meta);
            break;

        case 2:
            _notify_value_changed<uint16_t>(meta);
            break;

        case 1:
            _notify_value_changed<uint8_t>(meta);
            break;

        default:
            D_PRINTF("MQTT: Unsupported value size: %u\n", meta.value_size);
            return;
    }
}