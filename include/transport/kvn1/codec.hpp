#pragma once
#include "types.hpp"
#include <cstdint>
#include <string>
#include <string_view>

namespace kvn1 {

bool tlv_append_bytes(std::string& out, kvn1::FieldType t, const std::string& bytes);
void tlv_append_fixed8(std::string& out, kvn1::FieldType type, const uint8_t bytes[8]);
void tlv_append_i64(std::string& out, int64_t x);
void tlv_append_f64(std::string& out, double x);

bool tlv_read_i64(std::string_view v, int64_t& out);
bool tlv_read_f64(std::string_view v, double& out);

bool tlv_pop_one(std::string_view& payload, kvn1::FieldType& t, std::string_view& v);

void append_u32_be(std::string& out, uint32_t x);
uint32_t read_u32_be(const uint8_t* p);

std::string encode_frame(kvn1::RespType type, const std::string& payload);

ParseFrameStatus try_parse_frame(std::string& inbuf, uint8_t& type, std::string& payload);

} // namespace kvn1
