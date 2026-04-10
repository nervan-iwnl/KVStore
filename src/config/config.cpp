#include "config/config.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdlib>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

auto trim(std::string_view value) -> std::string_view {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
        value.remove_prefix(1);
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
        value.remove_suffix(1);
    }
    return value;
}

auto strip_quotes(std::string_view value) -> std::string_view {
    value = trim(value);
    if (value.size() >= 2) {
        const bool quoted =
            (value.front() == '"' && value.back() == '"') ||
            (value.front() == '\'' && value.back() == '\'');
        if (quoted) {
            value.remove_prefix(1);
            value.remove_suffix(1);
        }
    }
    return value;
}

auto env_key(std::string_view prefix, std::string_view name) -> std::string {
    return std::string(prefix) + std::string(name);
}

auto read_env(std::string_view prefix, std::string_view name) -> std::optional<std::string> {
    const std::string key = env_key(prefix, name);
    if (const char* value = std::getenv(key.c_str()); value != nullptr && value[0] != '\0') {
        return std::string(value);
    }
    return std::nullopt;
}

template <typename TInt>
auto parse_integral(std::string_view raw) -> std::optional<TInt> {
    raw = trim(raw);
    if (raw.empty()) {
        return std::nullopt;
    }

    TInt value{};
    const char* begin = raw.data();
    const char* end = begin + raw.size();
    const auto [ptr, ec] = std::from_chars(begin, end, value);
    if (ec != std::errc{} || ptr != end) {
        return std::nullopt;
    }
    return value;
}

auto lowercase(std::string_view raw) -> std::string {
    std::string value(raw);
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        }
    );
    return value;
}

auto parse_log_level(std::string_view raw) -> std::optional<kvd::config::LogLevel> {
    const std::string value = lowercase(strip_quotes(raw));
    if (value == "error") {
        return kvd::config::LogLevel::Error;
    }
    if (value == "warn" || value == "warning") {
        return kvd::config::LogLevel::Warn;
    }
    if (value == "info") {
        return kvd::config::LogLevel::Info;
    }
    if (value == "debug") {
        return kvd::config::LogLevel::Debug;
    }
    return std::nullopt;
}

auto parse_ttl_wheels(std::string_view raw) -> std::optional<std::vector<std::uint32_t>> {
    raw = strip_quotes(raw);
    if (raw.empty()) {
        return std::nullopt;
    }

    std::vector<std::uint32_t> wheels;
    while (!raw.empty()) {
        const auto pos = raw.find(',');
        const auto token = trim(raw.substr(0, pos));
        const auto parsed = parse_integral<std::uint32_t>(token);
        if (!parsed.has_value()) {
            return std::nullopt;
        }
        wheels.push_back(*parsed);

        if (pos == std::string_view::npos) {
            break;
        }
        raw.remove_prefix(pos + 1);
    }

    return wheels.empty() ? std::nullopt : std::optional<std::vector<std::uint32_t>>{std::move(wheels)};
}

auto make_parse_error(std::string_view key, std::string_view value, std::string_view expected) -> std::runtime_error {
    return std::runtime_error(
        "invalid environment override " + std::string(key) +
        "=" + std::string(value) + " (expected " + std::string(expected) + ")"
    );
}

void assign_string_env(std::string& field, std::string_view prefix, std::string_view name) {
    if (auto raw = read_env(prefix, name)) {
        field = std::string(strip_quotes(*raw));
    }
}

template <typename TInt>
void assign_integral_env(
    TInt& field,
    std::string_view prefix,
    std::string_view name,
    std::string_view expected_name
) {
    if (auto raw = read_env(prefix, name)) {
        const auto parsed = parse_integral<TInt>(*raw);
        if (!parsed.has_value()) {
            throw make_parse_error(env_key(prefix, name), *raw, expected_name);
        }
        field = *parsed;
    }
}

void assign_log_level_env(kvd::config::LoggingConfig& logging, std::string_view prefix) {
    if (auto raw = read_env(prefix, "LOG_LEVEL")) {
        const auto parsed = parse_log_level(*raw);
        if (!parsed.has_value()) {
            throw make_parse_error(env_key(prefix, "LOG_LEVEL"), *raw, "error|warn|info|debug");
        }
        logging.level = *parsed;
    }
}

void assign_ttl_wheels_env(kvd::config::TtlConfig& ttl, std::string_view prefix) {
    if (auto raw = read_env(prefix, "TTL_WHEELS")) {
        const auto parsed = parse_ttl_wheels(*raw);
        if (!parsed.has_value()) {
            throw make_parse_error(env_key(prefix, "TTL_WHEELS"), *raw, "comma-separated uint32 list");
        }
        ttl.wheels = std::move(*parsed);
    }
}

void apply_identity_env(kvd::config::AppConfigBuilder& builder, std::string_view prefix) {
    assign_string_env(builder.identity().server_version, prefix, "SERVER_VERSION");
}

void apply_listener_env(kvd::config::AppConfigBuilder& builder, std::string_view prefix) {
    assign_string_env(builder.listener().bind_address, prefix, "BIND_ADDRESS");
    assign_integral_env(builder.listener().port, prefix, "PORT", "int");
    assign_integral_env(builder.listener().listen_backlog, prefix, "LISTEN_BACKLOG", "int");
    assign_integral_env(builder.listener().max_connections, prefix, "MAX_CONNECTIONS", "int");
}

void apply_transport_env(kvd::config::AppConfigBuilder& builder, std::string_view prefix) {
    assign_integral_env(builder.transport().max_frame_bytes, prefix, "MAX_FRAME_BYTES", "size_t");
    assign_integral_env(builder.transport().max_pending_out_bytes, prefix, "MAX_PENDING_OUT_BYTES", "size_t");
    assign_integral_env(builder.transport().max_pending_conn_bytes, prefix, "MAX_PENDING_CONN_BYTES", "size_t");
    assign_integral_env(builder.transport().max_requests_per_iteration, prefix, "MAX_REQUESTS_PER_ITERATION", "size_t");
    assign_integral_env(builder.transport().stream_chunk_bytes, prefix, "STREAM_CHUNK_BYTES", "size_t");
    assign_integral_env(builder.transport().stream_window_bytes, prefix, "STREAM_WINDOW_BYTES", "size_t");
    assign_integral_env(builder.transport().max_streams_per_connection, prefix, "MAX_STREAMS_PER_CONNECTION", "size_t");
    assign_integral_env(builder.transport().max_scan_page_keys, prefix, "MAX_SCAN_PAGE_KEYS", "size_t");
    assign_integral_env(builder.transport().io_uring_queue_depth, prefix, "IO_URING_QUEUE_DEPTH", "int");
}

void apply_store_env(kvd::config::AppConfigBuilder& builder, std::string_view prefix) {
    assign_integral_env(builder.store().shard_count, prefix, "STORE_SHARD_COUNT", "size_t");
}

void apply_ttl_env(kvd::config::AppConfigBuilder& builder, std::string_view prefix) {
    assign_integral_env(builder.ttl().tick_ms, prefix, "TTL_TICK_MS", "int64");
    assign_ttl_wheels_env(builder.ttl(), prefix);
}

void apply_durability_env(kvd::config::AppConfigBuilder& builder, std::string_view prefix) {
    assign_string_env(builder.durability().wal_dir, prefix, "WAL_DIR");
    assign_string_env(builder.durability().snapshot_dir, prefix, "SNAPSHOT_DIR");
    assign_integral_env(builder.durability().wal_segment_max_bytes, prefix, "WAL_SEGMENT_MAX_BYTES", "size_t");
    assign_integral_env(builder.durability().fsync_interval_ms, prefix, "FSYNC_INTERVAL_MS", "int");
}

void apply_security_env(kvd::config::AppConfigBuilder& builder, std::string_view prefix) {
    assign_string_env(builder.security().auth_token, prefix, "AUTH_TOKEN");
}

void apply_logging_env(kvd::config::AppConfigBuilder& builder, std::string_view prefix) {
    assign_log_level_env(builder.logging(), prefix);
}

void apply_environment(kvd::config::AppConfigBuilder& builder, std::string_view prefix) {
    apply_identity_env(builder, prefix);
    apply_listener_env(builder, prefix);
    apply_transport_env(builder, prefix);
    apply_store_env(builder, prefix);
    apply_ttl_env(builder, prefix);
    apply_durability_env(builder, prefix);
    apply_security_env(builder, prefix);
    apply_logging_env(builder, prefix);
}

} // namespace

namespace kvd::config {

auto build_app_config(AppConfigBootstrapOptions options) -> AppConfig {
    return build_app_config(AppConfig::builder(), std::move(options));
}

auto build_app_config(AppConfigBuilder builder, AppConfigBootstrapOptions options) -> AppConfig {
    if (options.apply_environment) {
        apply_environment(builder, options.environment_prefix);
    }

    const auto spec = options.validate ? builder.build_checked() : builder.build();
    return AppConfig{spec};
}

auto format_config_issues(const std::vector<ConfigIssue>& issues) -> std::string {
    std::ostringstream out;
    for (const auto& issue : issues) {
        out << " - " << issue.field_path << ": " << issue.message << '\n';
    }
    return out.str();
}

} // namespace kvd::config