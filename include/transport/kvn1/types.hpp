#pragma once
#include <cstdint>

namespace kvn1 {

inline constexpr char MAGIC[4] = {'K','V','N','1'};

inline constexpr uint32_t MAX_FRAME_LEN = 16u * 1024u * 1024u; // 16 MiB

enum class MsgType : std::uint8_t {
#define CMD(Name, WireId, Op, Str, Feat, Flags, Since, SchemaId, Handler) Name = WireId,
#include "spec/commands.def"
#undef CMD
};

enum class RespType : uint8_t {
    Ok    = 0x81,
    Err   = 0x82,
    Value = 0x83,
    Int   = 0x84,
    Float = 0x85,
    Null  = 0x86
};

enum class FieldType : uint8_t {
    Key   = 1,
    Value = 2,
    I64   = 3,
    F64   = 4
};

enum class ParseFrameStatus : uint8_t {
    Ok,       
    NeedMore, 
    BadFrame  
};
} // namespace kvn1
