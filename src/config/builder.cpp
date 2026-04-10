#include "config/builder.hpp"

#include "util/utils.hpp"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void add_issue(
    std::vector<kvd::config::ConfigIssue>& issues,
    std::string_view path,
    std::string_view message
) {
    issues.push_back({std::string(path), std::string(message)});
}

auto validate_spec(const kvd::config::AppConfigSpec& spec) -> std::vector<kvd::config::ConfigIssue> {
    std::vector<kvd::config::ConfigIssue> issues;

    if (spec.listener.bind_address.empty()) {
        add_issue(issues, "listener.bind_address", "must not be empty");
    }
    if (spec.listener.port <= 0 || spec.listener.port > 65535) {
        add_issue(issues, "listener.port", "must be in range 1..65535");
    }
    if (spec.listener.listen_backlog <= 0) {
        add_issue(issues, "listener.listen_backlog", "must be positive");
    }
    if (spec.listener.max_connections <= 0) {
        add_issue(issues, "listener.max_connections", "must be positive");
    }

    if (spec.transport.max_frame_bytes == 0) {
        add_issue(issues, "transport.max_frame_bytes", "must be positive");
    }

    if (spec.store.shard_count == 0) {
        add_issue(issues, "store.shard_count", "must be positive");
    }

    if (spec.ttl.tick_ms <= 0) {
        add_issue(issues, "ttl.tick_ms", "must be positive");
    }
    if (spec.ttl.wheels.empty()) {
        add_issue(issues, "ttl.wheels", "must not be empty");
    }
    for (std::size_t i = 0; i < spec.ttl.wheels.size(); ++i) {
        if (!is_pow2(spec.ttl.wheels[i])) {
            add_issue(issues, "ttl.wheels[" + std::to_string(i) + "]", "must be a power of two");
        }
    }

    if (spec.durability.wal_dir.empty()) {
        add_issue(issues, "durability.wal_dir", "must not be empty");
    }
    if (spec.durability.snapshot_dir.empty()) {
        add_issue(issues, "durability.snapshot_dir", "must not be empty");
    }

    return issues;
}

} // namespace

namespace kvd::config {

ConfigValidationError::ConfigValidationError(std::vector<ConfigIssue> issues)
    : std::runtime_error("invalid kvd app config")
    , issues_(std::move(issues)) {}

auto ConfigValidationError::issues() const noexcept -> const std::vector<ConfigIssue>& {
    return issues_;
}

auto AppConfigBuilder::validate() const -> std::vector<ConfigIssue> {
    return validate_spec(draft_);
}

auto AppConfigBuilder::build_checked() const -> AppConfigSpec {
    auto issues = validate();
    if (!issues.empty()) {
        throw ConfigValidationError{std::move(issues)};
    }
    return draft_;
}

} // namespace kvd::config