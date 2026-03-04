#pragma once
#include <string>
#include <variant>
#include <cstdint>

#include "spec/schema.hpp"

enum class Op : std::uint16_t {
#define CMD(Name, WireId, OpExpr, Str, Feat, Flags, SinceKvn, SchemaId, Handler) Name,
#include "spec/commands.def"
#undef CMD
};

struct CoreReq {
    Op op;
    std::string key;
    std::variant<std::monostate, std::string, int64_t, double> arg;
    Mask fields_seen = 0;
};

enum class RespKind { Ok, Err, Null, Value, Int, Float };

struct CoreResp {
    RespKind kind;
    std::variant<std::monostate, std::string, int64_t, double> data; 
};
