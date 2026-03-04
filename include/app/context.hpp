#pragma once
#include <cstddef>
#include <string>

class KVStore;
class TtlCleaner;


enum class LogLevel {Error, Warn, Info, Debug};

struct AppConfig {
    bool protocol_strict_mode = true;

    std::size_t max_frame_size = 1024 * 1024;
    std::size_t max_tlv_field_size = 256 * 1024;
    std::size_t max_tlv_count = 16;
    std::size_t max_pending_out_bytes = 4 * 1024 * 1024;
    std::size_t max_requests_per_iteration = 64;

    LogLevel log_level = LogLevel::Info;

    std::string server_version = "0.1.0";
    std::string protocol_version = "KVN/1";
};

struct AppContext {
    KVStore& store;
    TtlCleaner& cleaner;
    const AppConfig& config;
};