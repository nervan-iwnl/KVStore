#pragma once
#include "app/command.hpp"
#include <string>
#include <cstdint>

bool decode_kvn1_request(uint8_t raw_msg_type, const std::string& payload, CoreReq& out);

void encode_kvn1_response(const CoreResp& resp, std::string& outbuf);
