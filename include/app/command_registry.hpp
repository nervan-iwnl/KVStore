#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "command.hpp" 
#include "spec/schema.hpp"

struct AppContext;


using CommandHandler = CoreResp (*)(const CoreReq&, AppContext&);

enum class FeatureGroup : std::uint8_t {
    Proto,      // HELLO / protocol basics
    CoreKV,     // GET/SET/DEL/SIZE
    Numeric,    // INCR/INCRBY/INCRBYFLOAT
    Ttl,        // PEXPIRE/PTTL
    Admin       // INFO/STATS/COMMANDS (потом)
};


enum class CommandFlags : std::uint32_t {
    None       = 0,
    Read       = 1u << 0,
    Write      = 1u << 1,
    Admin      = 1u << 2,
    Idempotent = 1u << 3,
    Hidden     = 1u << 4,
};

constexpr CommandFlags operator|(CommandFlags a, CommandFlags b) noexcept {
    return static_cast<CommandFlags>(
        static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}

constexpr CommandFlags operator&(CommandFlags a, CommandFlags b) noexcept {
    return static_cast<CommandFlags>(
        static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b));
}

constexpr bool has_flag(CommandFlags flags, CommandFlags bit) noexcept {
    return static_cast<std::uint32_t>(flags & bit) != 0;
}



struct FeatureSpec {
    FeatureGroup f;
    std::string_view cap_v1;
    uint8_t since = 1;
};


struct CommandSpec {
    Op op;
    std::string_view name;     
    CommandHandler handler;
    const FeatureSpec* feature;
    CommandFlags flags;
    std::uint16_t since_kvn;   
    SchemaId schema_id;
};


namespace command_registry {

const CommandSpec* find_by_name(std::string_view name) noexcept;

const CommandSpec* find_by_op(Op op) noexcept;

const std::vector<CommandSpec>& all() noexcept;

std::vector<FeatureGroup> collect_features();            
std::vector<std::string_view> collect_caps_v1();         

std::string_view op_to_string(Op op) noexcept;

std::string flags_to_text(CommandFlags flags);

} // namespace command_registry