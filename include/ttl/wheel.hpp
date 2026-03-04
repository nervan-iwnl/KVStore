#pragma once
#include <cstdint>
#include <vector>
#include "ttl/types.hpp"

class HierTtlWheel {
public:
    struct Config {
        int64_t tick_ms = 10;                 
        std::vector<uint32_t> wheels;         
    };

    explicit HierTtlWheel(Config cfg);

    int64_t tick_ms() const noexcept { return tick_ms_; }
    const std::vector<uint32_t>& wheels() const noexcept { return wheels_; }

    void schedule(TimerItem item, int64_t now_ms);

    void tick_once(int64_t now_ms, std::vector<TimerItem>& out_due);

private:
    // cfg
    int64_t tick_ms_ = 10;
    std::vector<uint32_t> wheels_;   
    std::vector<uint32_t> masks_;    
    std::vector<uint64_t> step_;     

    std::vector<uint32_t> cur_;     

    std::vector<std::vector<std::vector<TimerItem>>> levels_;

    void validate_and_init(); 
    void place_item(uint64_t delta_ticks, TimerItem item);
    void cascade_from_level(size_t level, int64_t now_ms); 
};

