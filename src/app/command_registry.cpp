#include <algorithm>

#include "app/command_registry.hpp"
#include "app/handlers/handlers_admin.hpp"
#include "app/handlers/handlers_kv.hpp"
#include "app/handlers/handlers_numeric.hpp"
#include "app/handlers/handlers_ttl.hpp"
namespace command_registry{

namespace {
inline constexpr FeatureSpec FEAT_PROTO {FeatureGroup::Proto,   "proto.kvn1",     1};
inline constexpr FeatureSpec FEAT_CORE  {FeatureGroup::CoreKV,  "core.kv",        1};
inline constexpr FeatureSpec FEAT_NUM   {FeatureGroup::Numeric, "numeric.basic",  1};
inline constexpr FeatureSpec FEAT_TTL   {FeatureGroup::Ttl,     "ttl.basic",      1};
inline constexpr FeatureSpec FEAT_ADMIN {FeatureGroup::Admin,   "admin.basic",    1};

constexpr const FeatureSpec* feat(FeatureGroup f) noexcept {
    switch (f) {
        case FeatureGroup::Proto:   return &FEAT_PROTO;
        case FeatureGroup::CoreKV:  return &FEAT_CORE;
        case FeatureGroup::Numeric: return &FEAT_NUM;
        case FeatureGroup::Ttl:     return &FEAT_TTL;
        case FeatureGroup::Admin:   return &FEAT_ADMIN;
        default: return nullptr;
    }
}



const std::vector<CommandSpec> kSpecs = {
#define CMD(Name, WireId, OpExpr, Str, Feat, Flags, SinceKvn, Schema, Handler) \
  { OpExpr, Str, Handler, feat(Feat), Flags, (std::uint16_t)SinceKvn, Schema},
#include "spec/commands.def"
#undef CMD
};
} // namespace

const CommandSpec* find_by_name(std::string_view name) noexcept {   
    for (const auto& cmd : kSpecs) {
        if (cmd.name == name) {
            return &cmd;
        }
    }
    return nullptr;
}

const CommandSpec* find_by_op(Op op) noexcept {
    for (const auto& cmd : kSpecs) {
        if (cmd.op == op) {
            return &cmd;
        }
    }
    return nullptr;
}

const std::vector<CommandSpec>& all() noexcept {
    return kSpecs;
}

std::vector<FeatureGroup> collect_features() {
    std::vector<FeatureGroup> features;
    for (const auto& cmd : kSpecs) {
        if (!cmd.feature) continue;
        auto f = cmd.feature->f;
        if (std::find(features.begin(), features.end(), f) == features.end()) {
            features.push_back(f);
        }
    }
    return features;
}

std::vector<std::string_view> collect_caps_v1() {
    std::vector<std::string_view> caps;
    for (auto f : collect_features()) {
        if (f == FeatureGroup::Proto) continue;
        if (auto s = feat(f)) caps.push_back(s->cap_v1);
    }
    return caps;
}

std::string_view op_to_string(Op op) noexcept {
    for (const auto& cmd : kSpecs) {
        if (cmd.op == op) return cmd.name;
        
    }
    return "";
}

std::string flags_to_text(CommandFlags flags) {
    std::string out;

    auto add = [&](const char* s) {
        if (!out.empty()) out += ',';
        out += s;
    };

    if (has_flag(flags, CommandFlags::Read))       add("read");
    if (has_flag(flags, CommandFlags::Write))      add("write");
    if (has_flag(flags, CommandFlags::Admin))      add("admin");
    if (has_flag(flags, CommandFlags::Idempotent)) add("idempotent");
    if (has_flag(flags, CommandFlags::Hidden))     add("hidden");

    return out;
}

}
