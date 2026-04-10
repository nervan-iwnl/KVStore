#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace kvd::config {

enum class LogLevel {
    Error,
    Warn,
    Info,
    Debug,
};

struct IdentityConfig {
    std::string server_version = "0.1.1";
};

struct ListenerConfig {
    std::string bind_address = "0.0.0.0";
    int port = 7777;
    int listen_backlog = 64;
    int max_connections = 256;
};

struct TransportConfig {
    std::size_t max_frame_bytes = 1024 * 1024;
    std::size_t max_pending_out_bytes = 4 * 1024 * 1024;
    std::size_t max_pending_conn_bytes = 16 * 1024 * 1024;
    std::size_t max_requests_per_iteration = 64;
    std::size_t stream_chunk_bytes = 256 * 1024;
    std::size_t stream_window_bytes = 4 * 1024 * 1024;
    std::size_t max_streams_per_connection = 128;
    std::size_t max_scan_page_keys = 512;
    int io_uring_queue_depth = 256;
};

struct StoreConfig {
    std::size_t shard_count = 64;
};

struct TtlConfig {
    std::int64_t tick_ms = 10;
    std::vector<std::uint32_t> wheels = {1024, 1024, 1024};
};

struct DurabilityConfig {
    std::string wal_dir = "./var/wal";
    std::string snapshot_dir = "./var/snapshot";
    std::size_t wal_segment_max_bytes = 64 * 1024 * 1024;
    int fsync_interval_ms = 1000;
};

struct SecurityConfig {
    std::string auth_token;
};

struct LoggingConfig {
    LogLevel level = LogLevel::Info;
};

struct ConfigIssue {
    std::string field_path;
    std::string message;
};

} // namespace kvd::config