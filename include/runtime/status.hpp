#pragma once

#include <string>
#include <utility>
#include "kvd/api/v1/envelope.pb.h"

namespace kvd::runtime {

struct Status {
    bool ok = true;
    ::kvd::api::v1::ProtoError::Code code =
        ::kvd::api::v1::ProtoError::CODE_UNSPECIFIED;
    std::string message;
};

inline Status ok() {
    return {};
}

inline Status error(::kvd::api::v1::ProtoError::Code code, std::string message) {
    Status err;
    err.ok = false;
    err.message = std::move(message);
    err.code = code;
    return err;
}

inline Status not_found(std::string message) {
    return error(::kvd::api::v1::ProtoError::NOT_FOUND, std::move(message));
}

inline Status bad_request(std::string message) {
    return error(::kvd::api::v1::ProtoError::BAD_REQUEST, std::move(message));
}

inline Status internal_error(std::string message) {
    return error(::kvd::api::v1::ProtoError::INTERNAL, std::move(message));
}

inline Status unknown_method(std::string message) {
    return error(::kvd::api::v1::ProtoError::UNKNOWN_METHOD, std::move(message));
}

} // namespace kvd::runtime

