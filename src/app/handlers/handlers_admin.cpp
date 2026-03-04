#include "app/handlers/handlers_admin.hpp"
#include "app/response.hpp"
#include "app/context.hpp"
#include "domain/kv_store.hpp"
#include "app/command_registry.hpp"

#include <sstream>
#include <iomanip>


CoreResp handle_ping(const CoreReq&, AppContext& ) {
    return ok();
}


CoreResp handle_info(const CoreReq&, AppContext& ctx) {
    std::ostringstream ss;
    ss << "server_version=" << ctx.config.server_version << '\n';
    ss << "protocol_version=" << ctx.config.protocol_version << '\n';
    ss << "keys=" << ctx.store.size() << '\n';
    ss << "strict_mode=" << (ctx.config.protocol_strict_mode ? 1 : 0) << '\n';
    ss << "max_frame_size=" << ctx.config.max_frame_size << '\n';
    ss << "max_tlv_field_size=" << ctx.config.max_tlv_field_size << '\n';
    ss << "max_tlv_count=" << ctx.config.max_tlv_count;
    return val(ss.str());
}

CoreResp handle_caps(const CoreReq&, AppContext&) {
    std::ostringstream ss;
    for (auto& cap : command_registry::collect_caps_v1()) {
        ss << cap << '\n';
    }
    return val(ss.str());
}

CoreResp handle_commands(const CoreReq&, AppContext&) {
    constexpr int W_NAME    = 14;
    constexpr int W_FEATURE = 16;
    constexpr int W_SINCE   = 8;

    std::ostringstream ss;

     ss << std::left
       << std::setw(W_NAME)    << "NAME"
       << std::setw(W_FEATURE) << "FEATURE"
       << std::setw(W_SINCE)   << "SINCE"
       << "FLAGS"
       << '\n';

    ss << std::string(W_NAME + W_FEATURE + W_SINCE + 20, '-');

    for (const auto& cmd : command_registry::all()) {
        if (has_flag(cmd.flags, CommandFlags::Hidden)) continue;

        ss << '\n'
           << std::left
           << std::setw(W_NAME)    << cmd.name
           << std::setw(W_FEATURE) << (cmd.feature ? cmd.feature->cap_v1 : "unknown")
           << std::setw(W_SINCE)   << cmd.since_kvn
           <<  command_registry::flags_to_text(cmd.flags);
    }  

    return val(ss.str());
}