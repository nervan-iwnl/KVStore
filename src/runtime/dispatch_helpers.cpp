#include "runtime/dispatch_helpers.hpp"

namespace kvd::runtime {

void fill_error(::kvd::api::v1::ResponseFrame& out,
                ::kvd::api::v1::ProtoError::Code code,
                const std::string& message) {
    out.set_ok(false);
    out.clear_payload();
    auto* err = out.mutable_error();
    err->set_code(code);
    err->set_message(message);
}

bool fill_serialized_payload(::kvd::api::v1::ResponseFrame& out,
                             const google::protobuf::Message& msg) {
    std::string bytes;
    if (!msg.SerializeToString(&bytes)) return false;
    out.set_ok(true);
    out.clear_error();
    out.set_payload(bytes);
    return true;
}

} // namespace kvd::runtime