#include "ttl/wheel.hpp"
#include "ttl/types.hpp"
#include "util/utils.hpp"
#include <stdexcept>
#include <limits>
#include <iterator>
#include <utility>

HierTtlWheel::HierTtlWheel(Config cfg) {
    tick_ms_ = cfg.tick_ms;
    if (tick_ms_ < 1) throw  std::invalid_argument("tick_ms must be positive");
    if (cfg.wheels.empty()) throw std::invalid_argument("wheel must have at least one level");
    wheels_ = std::move(cfg.wheels);
    validate_and_init();
}

void HierTtlWheel::validate_and_init() {
    cur_.resize(wheels_.size(), 0);
    levels_.resize(wheels_.size());
    step_.resize(wheels_.size());
    masks_.resize(wheels_.size());
    for (size_t i = 0; i < wheels_.size(); ++i) {
        if (!is_pow2(wheels_[i])) throw std::invalid_argument("wheel sizes must be power of two");
        levels_[i].resize(wheels_[i]);
        if (i > 0) {
            if (step_[i - 1] > std::numeric_limits<uint64_t>::max() / wheels_[i-1]) throw std::overflow_error("wheel configuration overflow: step exceeds uint64_t range");
            step_[i] = step_[i - 1] * wheels_[i - 1];
        } else step_[i] = 1;
        masks_[i] = wheels_[i] - 1;
    }
    if (step_.back() > std::numeric_limits<uint64_t>::max() / wheels_.back()) throw std::overflow_error("wheel configuration overflow: total range exceeds uint64_t");
}

void HierTtlWheel::place_item(uint64_t delta_ticks, TimerItem item) {
    size_t lvl = wheels_.size() - 1;
    for (size_t i = 0; i < wheels_.size(); ++i) {
        if (delta_ticks / step_[i] < wheels_[i]) {
            lvl = i;
            break;
        }
    }
    levels_[lvl][(cur_[lvl] + (delta_ticks / step_[lvl])) & masks_[lvl]].push_back(std::move(item));
}

void HierTtlWheel::cascade_from_level(size_t level, int64_t now_ms) {
    if (level == 0) return;
    cur_[level] = (cur_[level] + 1) & masks_[level];
    auto slot = cur_[level];
    auto bucket = std::move(levels_[level][slot]);
    for (auto &el : bucket) {
        auto remaining_ms = el.expire_at_ms - now_ms;
        if (remaining_ms <= 0) remaining_ms = 0;
        schedule(std::move(el), now_ms);
    }
}

void HierTtlWheel::tick_once(int64_t now_ms, std::vector<TimerItem>& out_due) {
    size_t i = 0;
    cur_[i] = (cur_[i] + 1) & masks_[i];
    auto& lvl0 = levels_[0][cur_[0]];
    while (cur_[i] == 0) {
        i++;
        if (i == wheels_.size()) break;
        cascade_from_level(i, now_ms);
    }
    std::move(lvl0.begin(), lvl0.end(), std::back_inserter(out_due));
    lvl0.clear();
}

void HierTtlWheel::tick_n(size_t steps, int64_t first_tick_ms, std::vector<TimerItem>& out_due) {
    out_due.clear();

    const auto tick = tick_ms_;
    for (size_t s = 0; s < steps; ++s) {
        tick_once(first_tick_ms + static_cast<int64_t>(s) * tick, out_due);
    }
}

void HierTtlWheel::schedule(TimerItem item, int64_t now_ms) {
    auto remaining_ms = item.expire_at_ms - now_ms;
    if (remaining_ms < 0) remaining_ms = 1;
    int64_t delta_ticks = remaining_ms / tick_ms_;
    if (delta_ticks < 1) delta_ticks = 1;
    place_item(delta_ticks, std::move(item));
}