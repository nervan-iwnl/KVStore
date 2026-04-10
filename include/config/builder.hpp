#pragma once

#include "config/spec.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

namespace kvd::config {

class ConfigValidationError final : public std::runtime_error {
public:
    explicit ConfigValidationError(std::vector<ConfigIssue> issues);

    auto issues() const noexcept -> const std::vector<ConfigIssue>&;

private:
    std::vector<ConfigIssue> issues_;
};

class AppConfigBuilder {
public:
    AppConfigBuilder() = default;

    explicit AppConfigBuilder(AppConfigSpec base)
        : draft_(std::move(base)) {}

    static auto from(AppConfigSpec base) -> AppConfigBuilder {
        return AppConfigBuilder{std::move(base)};
    }

    auto identity() noexcept -> IdentityConfig& { return draft_.identity; }
    auto listener() noexcept -> ListenerConfig& { return draft_.listener; }
    auto transport() noexcept -> TransportConfig& { return draft_.transport; }
    auto store() noexcept -> StoreConfig& { return draft_.store; }
    auto ttl() noexcept -> TtlConfig& { return draft_.ttl; }
    auto durability() noexcept -> DurabilityConfig& { return draft_.durability; }
    auto security() noexcept -> SecurityConfig& { return draft_.security; }
    auto logging() noexcept -> LoggingConfig& { return draft_.logging; }

    auto spec() const noexcept -> const AppConfigSpec& {
        return draft_;
    }

    auto validate() const -> std::vector<ConfigIssue>;

    [[nodiscard]] auto build() const -> AppConfigSpec {
        return draft_;
    }

    [[nodiscard]] auto build_checked() const -> AppConfigSpec;

private:
    AppConfigSpec draft_{};
};

} // namespace kvd::config