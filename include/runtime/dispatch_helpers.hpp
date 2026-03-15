#pragma once
#include <string>
#include <google/protobuf/message.h>
#include "kvd/api/v1/envelope.pb.h"

namespace kvd::runtime {

void fill_error(::kvd::api::v1::ResponseFrame& out,
                ::kvd::api::v1::ProtoError::Code code,
                const std::string& message);

bool fill_serialized_payload(::kvd::api::v1::ResponseFrame& out,
                             const google::protobuf::Message& msg);

} // namespace kvd::runtime
