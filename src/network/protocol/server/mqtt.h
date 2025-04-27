#include <AsyncMqttClient.h>

#include "application.h"
#include "metadata.h"
#include "constants.h"
#include "credentials.h"
#include "debug.h"

enum class MqttServerState : uint8_t {
    UNINITIALIZED,
    CONNECTING,
    CONNECTED,
    DISCONNECTED
};

class MqttServer {
    AsyncMqttClient _mqttClient;

    Application &_app;

    MqttServerState _state = MqttServerState::UNINITIALIZED;
    unsigned long _state_change_time = 0;
    unsigned long _last_connection_attempt_time = 0;

public:
    virtual ~MqttServer() = default;
    explicit MqttServer(Application &app);

    void begin();

    void handle_connection();

protected:
    Application &app() { return _app; }

    void _on_connect(bool sessionPresent);
    void _on_disconnect(AsyncMqttClientDisconnectReason reason);
    void _on_message(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);

    void _subscribe(const char *topic, uint8_t qos);
    void _publish(const char *topic, uint8_t qos, const char *payload, size_t length);

    void _process_message(const String &topic, const String &payload);
    void _process_notification(NotificationProperty prop);

    void _change_state(MqttServerState state);
    void _connect();

    template<typename T>
    void _set_value(const PropertyMetadata &meta, const T &value);

    template<typename T>
    void _notify_value_changed(const PropertyMetadata &meta);


    virtual void _transform_topic_payload(const String &topic, String &payload) {};
    virtual void _transform_topic_notification(const PropertyMetadata &meta, String &payload) {};

    virtual void _after_message_process(const PropertyMetadata &meta);
};


template<typename T>
void MqttServer::_notify_value_changed(const PropertyMetadata &meta) {
    T value;
    memcpy(&value, (uint8_t *) &_app.config + meta.value_offset, sizeof(value));

    auto value_str = String(value);
    _transform_topic_notification(meta, value_str);

    _publish(meta.mqtt_out_topic, 1, value_str.c_str(), value_str.length());
}


template<typename T>
void MqttServer::_set_value(const PropertyMetadata &meta, const T &value) {
    memcpy((uint8_t *) &_app.config + meta.value_offset, &value, sizeof(value));

    D_PRINTF("Set %s = ", __debug_enum_str(meta.property));
    D_PRINT_HEX(((uint8_t *) (&value)), sizeof(value));
}
