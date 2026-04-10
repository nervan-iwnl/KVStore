#pragma once

#include "config/types.hpp"

namespace kvd::config {

struct AppConfigSpec {
    IdentityConfig identity{};
    ListenerConfig listener{};
    TransportConfig transport{};
    StoreConfig store{};
    TtlConfig ttl{};
    DurabilityConfig durability{};
    SecurityConfig security{};
    LoggingConfig logging{};
};

} // namespace kvd::config