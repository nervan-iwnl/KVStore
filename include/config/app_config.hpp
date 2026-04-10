#pragma once

#include "config/builder.hpp"

#include <utility>

namespace kvd::config {

class AppConfig {
public:
    AppConfig() = default;

    explicit AppConfig(AppConfigSpec spec)
        : spec_(std::move(spec)) {}

    explicit AppConfig(AppConfigBuilder builder)
        : AppConfig(builder.build()) {}

    AppConfig(const AppConfig&) = default;
    AppConfig(AppConfig&&) noexcept = default;
    auto operator=(const AppConfig&) -> AppConfig& = default;
    auto operator=(AppConfig&&) noexcept -> AppConfig& = default;

    static auto builder() -> AppConfigBuilder {
        return AppConfigBuilder{};
    }

    auto spec() const noexcept -> const AppConfigSpec& {
        return spec_;
    }

    auto rebuild() const -> AppConfigBuilder {
        return AppConfigBuilder::from(spec_);
    }

    auto identity() const noexcept -> const IdentityConfig& { return spec_.identity; }
    auto listener() const noexcept -> const ListenerConfig& { return spec_.listener; }
    auto transport() const noexcept -> const TransportConfig& { return spec_.transport; }
    auto store() const noexcept -> const StoreConfig& { return spec_.store; }
    auto ttl() const noexcept -> const TtlConfig& { return spec_.ttl; }
    auto durability() const noexcept -> const DurabilityConfig& { return spec_.durability; }
    auto security() const noexcept -> const SecurityConfig& { return spec_.security; }
    auto logging() const noexcept -> const LoggingConfig& { return spec_.logging; }

private:
    AppConfigSpec spec_{};
};

} // namespace kvd::config

using LogLevel = kvd::config::LogLevel;
using AppConfig = kvd::config::AppConfig;