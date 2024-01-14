#pragma once

#include "config.h"
#include "../data.h"

enum class ResponseType : uint8_t {
    CODE,
    STRING,
    BINARY,
};

enum class ResponseCode : uint8_t {
    OK,
    BAD_REQUEST,
    BAD_COMMAND,
    INTERNAL_ERROR
};

struct Response {
    ResponseType type;

    union {
        ResponseCode code;
        const char *str;

        const struct {
            uint16_t size;
            uint8_t *data;
        } buffer;
    } body;

    const char *codeString();

    inline bool isOk() { return type != ResponseType::CODE || body.code == ResponseCode::OK; }

    inline static Response ok() {
        return code(ResponseCode::OK);
    };

    inline static Response code(ResponseCode code) {
        return Response{.type = ResponseType::CODE, .body = {.code = code}};
    };
};

class ServerBase {
    AppConfig &_app_config;

public:
    explicit ServerBase(AppConfig &config);
    virtual ~ServerBase() = default;

    virtual void handle_incoming_data() = 0;

protected:
    inline AppConfig &app_config() { return _app_config; }

    Response handle_packet_data(const byte *buffer, uint16_t length);

    Response update_parameter(const PacketHeader &header, const void *data);
    Response calibrate(const PacketHeader &header, const void *data);
    Response process_command(const PacketHeader &header);
    Response process_data_request(const PacketHeader &header);
};