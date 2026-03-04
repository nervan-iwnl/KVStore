#include "transport/kvn1/codec.hpp"
#include <arpa/inet.h>   
#include <cstring>       

namespace kvn1 {

void append_u32_be(std::string& out, uint32_t x) {
    uint32_t net = htonl(x);
    out.append(reinterpret_cast<const char*>(&net), 4);
}

uint32_t read_u32_be(const uint8_t* p) {
    uint32_t net = 0;
    std::memcpy(&net, p, 4);
    return ntohl(net);
}

std::string encode_frame(kvn1::RespType type, const std::string& payload) {
    std::string frame;
    frame.append(kvn1::MAGIC, 4);
    uint32_t len = 1u + static_cast<uint32_t>(payload.size());
    append_u32_be(frame, len);
    frame.push_back(static_cast<char>(type));
    frame += payload;
    return frame;
}

ParseFrameStatus try_parse_frame(std::string& inbuf, uint8_t& type, std::string& payload) {
    payload.clear();
    if (inbuf.size() < 8) return kvn1::ParseFrameStatus::NeedMore;
    if (std::memcmp(inbuf.data(), kvn1::MAGIC, 4) != 0) return kvn1::ParseFrameStatus::BadFrame;

    uint32_t len = read_u32_be(reinterpret_cast<const uint8_t*>(inbuf.data() + 4));
    if (len == 0 || len > kvn1::MAX_FRAME_LEN) return kvn1::ParseFrameStatus::BadFrame;

    if (inbuf.size() < 8u + static_cast<size_t>(len)) return kvn1::ParseFrameStatus::NeedMore;

    type = static_cast<uint8_t>(inbuf[8]);
    if (len > 1) {
        payload.assign(inbuf.data() + 9, static_cast<size_t>(len - 1));
    }

    inbuf.erase(0, 8u + static_cast<size_t>(len));
    return kvn1::ParseFrameStatus::Ok;
}

bool tlv_append_bytes(std::string& out, kvn1::FieldType t, const std::string& bytes) {
    if (bytes.size() > UINT32_MAX) return false;
    out.push_back(static_cast<char>(t));
    append_u32_be(out, static_cast<uint32_t>(bytes.size()));
    out += bytes;
    return true;
}

void tlv_append_fixed8(std::string& out, kvn1::FieldType type, const uint8_t bytes[8]) {
    out.push_back(static_cast<char>(type));
    append_u32_be(out, 8u);
    out.append(reinterpret_cast<const char*>(bytes), 8);
}

void tlv_append_i64(std::string& out, int64_t x) {
    uint64_t u = static_cast<uint64_t>(x);
    uint64_t be = __builtin_bswap64(u);
    uint8_t bytes[8];
    std::memcpy(bytes, &be, 8);
    tlv_append_fixed8(out, kvn1::FieldType::I64, bytes);
}

void tlv_append_f64(std::string& out, double x) {
    uint64_t u;
    std::memcpy(&u, &x, 8);
    uint64_t be = __builtin_bswap64(u);
    uint8_t bytes[8];
    std::memcpy(bytes, &be, 8);
    tlv_append_fixed8(out, kvn1::FieldType::F64, bytes);
}

bool tlv_read_i64(std::string_view v, int64_t& out) {
    if (v.size() != 8) return false;
    uint64_t be;
    std::memcpy(&be, v.data(), 8);
    uint64_t host = __builtin_bswap64(be);
    out = static_cast<int64_t>(host);
    return true;
}

bool tlv_read_f64(std::string_view v, double& out) {
    if (v.size() != 8) return false;
    uint64_t be;
    std::memcpy(&be, v.data(), 8);
    uint64_t host = __builtin_bswap64(be);
    std::memcpy(&out, &host, 8);
    return true;
}

bool tlv_pop_one(std::string_view& payload, kvn1::FieldType& t, std::string_view& v) {
    if (payload.size() < 5) return false;
    uint32_t len = read_u32_be(reinterpret_cast<const uint8_t*>(payload.data() + 1));
    if (payload.size() < 5u + static_cast<size_t>(len)) return false;
    t = static_cast<kvn1::FieldType>(static_cast<uint8_t>(payload[0]));
    v = std::string_view(payload.data() + 5, len);
    payload.remove_prefix(5u + static_cast<size_t>(len));
    return true;
}

} // namespace kvn1
