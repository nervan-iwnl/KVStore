#include "transport/kvn1/adapter.hpp"
#include "transport/kvn1/codec.hpp"
#include <string_view>
#include <variant>

static void append_internal_err(std::string& payload, kvn1::RespType& r) {
    r = kvn1::RespType::Err;
    kvn1::tlv_append_bytes(payload, kvn1::FieldType::Value, "ERR internal");
}

bool decode_kvn1_request(uint8_t raw_msg_type, const std::string& payload, CoreReq& out) {
    out = CoreReq{};

    auto type = static_cast<kvn1::MsgType>(raw_msg_type);
    switch (type) {
    #define CMD(Name, WireId, OpExpr, Str, Feat, Flags, SinceKvn, SchemaId, Handler) \
        case kvn1::MsgType::Name: out.op = Op::Name; break;
    #include "spec/commands.def"
    #undef CMD
    default: return false;
    }

    auto mark_once = [&](FieldId f) -> bool {
        Mask bit = B(f);
        if (out.fields_seen & bit) return false;
        out.fields_seen |= bit;
        return true;
    };


    std::string_view p(payload);
    kvn1::FieldType ft;
    std::string_view v;

    while (kvn1::tlv_pop_one(p, ft, v)) {
        switch (ft) {
            case kvn1::FieldType::Key:
                if (!mark_once(FieldId::Key)) return false;
                out.key.assign(v.begin(), v.end());
                break;
            case kvn1::FieldType::Value:
                if (!mark_once(FieldId::Value)) return false;
                out.arg = std::string(v.begin(), v.end());
                break;
            case kvn1::FieldType::I64: {
                int64_t x;
                if (!mark_once(FieldId::I64) || !kvn1::tlv_read_i64(v, x)) return false;
                out.arg = x;
                break;
            }
            case kvn1::FieldType::F64: {
                double x;
                if (!mark_once(FieldId::F64) || !kvn1::tlv_read_f64(v, x)) return false;
                out.arg = x;
                break;
            }
            default:
                return false;
        }
    }

    return p.empty();
}

void encode_kvn1_response(const CoreResp& resp, std::string& outbuf) {
    kvn1::RespType r = kvn1::RespType::Err;
    std::string payload;

    switch (resp.kind) {
        case RespKind::Ok:
            r = kvn1::RespType::Ok;
            break;
        case RespKind::Null:
            r = kvn1::RespType::Null;
            break;
        case RespKind::Err:
            r = kvn1::RespType::Err;
            if (auto* s = std::get_if<std::string>(&resp.data)) {
                kvn1::tlv_append_bytes(payload, kvn1::FieldType::Value, *s);
            } else {
                append_internal_err(payload, r);
            }
            break;
        case RespKind::Float:
            r = kvn1::RespType::Float;
            if (auto* d = std::get_if<double>(&resp.data)) {
                kvn1::tlv_append_f64(payload, *d);
            } else {
                append_internal_err(payload, r);
            }
            break;
        case RespKind::Value:
            r = kvn1::RespType::Value;
            if (auto* s = std::get_if<std::string>(&resp.data)) {
                kvn1::tlv_append_bytes(payload, kvn1::FieldType::Value, *s);
            } else {
                append_internal_err(payload, r);
            }
            break;
        case RespKind::Int:
            r = kvn1::RespType::Int;
            if (auto* i = std::get_if<int64_t>(&resp.data)) {
                kvn1::tlv_append_i64(payload, *i);
            } else {
                append_internal_err(payload, r);
            }
            break;
    }

    outbuf += kvn1::encode_frame(r, payload);
}
