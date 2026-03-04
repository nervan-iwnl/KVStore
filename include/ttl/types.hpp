#pragma once
#include <cstdint>
#include <string>

struct TimerItem {
    std::string key;
    int64_t expire_at_ms = 0;
    uint64_t gen = 0;
};

class ITtlTarget {
public:
    virtual ~ITtlTarget() = default;

    virtual bool erase_expired_if_match(
        const std::string& key,
        int64_t expected_expire_at_ms,
        uint64_t expected_gen,
        int64_t now_ms) = 0;
};