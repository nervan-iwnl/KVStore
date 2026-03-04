#pragma once
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <thread>
#include <vector>
#include <string> 
#include <mutex>
#include "ttl/wheel.hpp"


class TtlCleaner {
public:
    TtlCleaner(ITtlTarget& target, HierTtlWheel::Config wheel_cfg);

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
