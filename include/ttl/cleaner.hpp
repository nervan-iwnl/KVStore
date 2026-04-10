#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "config/app_config.hpp"
#include "ttl/wheel.hpp"

class TtlCleaner {
public:
    TtlCleaner(ITtlTarget& target, HierTtlWheel::Config wheel_cfg);

    explicit TtlCleaner(ITtlTarget& target, const kvd::config::AppConfigSpec& spec)
        : TtlCleaner(target, spec.ttl) {}

    explicit TtlCleaner(ITtlTarget& target, const kvd::config::AppConfig& app_cfg)
        : TtlCleaner(target, app_cfg.ttl()) {}

    void start();
    void stop();

    void on_pexpire(
        const std::string& key,
        int64_t expire_at_ms,
        uint64_t gen);

private:
    ITtlTarget& target_;
    HierTtlWheel wheel_;

    std::atomic<bool> stop_flag_{false};
    std::mutex mu_;
    std::condition_variable cv_;
    std::thread worker_;

    void run();
};