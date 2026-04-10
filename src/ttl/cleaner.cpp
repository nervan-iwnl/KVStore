#include "ttl/cleaner.hpp"

#include <algorithm>
#include <chrono>
#include <utility>

#include "ttl/wheel.hpp"
#include "util/time.hpp"

TtlCleaner::TtlCleaner(ITtlTarget& target, HierTtlWheel::Config wheel_cfg)
    : target_(target)
    , wheel_(std::move(wheel_cfg)) {}

void TtlCleaner::run() {
    std::unique_lock<std::mutex> lk(mu_);
    const int64_t tick = wheel_.tick_ms();
    
    int64_t next_wakeup_steady_ms = kvd::util::steady_now_ms() + tick;
    int64_t next_tick_unix_ms = kvd::util::unix_now_ms() + tick;

    std::vector<TimerItem> due;
    while (!stop_flag_.load()) {
        const int64_t steady_now_ms = kvd::util::steady_now_ms();
        const auto sleep_ms = std::max<int64_t>(0, next_wakeup_steady_ms - steady_now_ms);
        cv_.wait_for(lk, std::chrono::milliseconds(sleep_ms));
        if (stop_flag_.load(std::memory_order_relaxed)) {
            break;
        }

        const int64_t now_unix_ms = kvd::util::unix_now_ms();
        const int64_t now_steady_ms = kvd::util::steady_now_ms();
        if (now_unix_ms < next_tick_unix_ms) {
            continue;
        }

        const size_t steps = static_cast<size_t>((now_unix_ms - next_tick_unix_ms) / tick + 1);

        due.clear();
        wheel_.tick_n(steps, next_tick_unix_ms, due);
        next_tick_unix_ms += static_cast<int64_t>(steps) * tick;
        next_wakeup_steady_ms = now_steady_ms + tick;

        lk.unlock();

        std::vector<TimerItem> back_wheel;
        back_wheel.reserve(due.size());
        for (auto& item : due) {
            if (item.expire_at_ms <= now_unix_ms) {
                target_.erase_expired_if_match(item.key, item.expire_at_ms, item.gen, now_unix_ms);
            } else {
                back_wheel.push_back(std::move(item));
            }
        }

        lk.lock();
        for (auto& item : back_wheel) {
            wheel_.schedule(std::move(item), now_unix_ms);
        }
    }
}

void TtlCleaner::start() {
    stop_flag_ = false;
    if (worker_.joinable()) {
        return;
    }
    worker_ = std::thread(&TtlCleaner::run, this);
}

void TtlCleaner::stop() {
    stop_flag_ = true;
    cv_.notify_one();
    if (worker_.joinable()) {
        worker_.join();
    }
}

void TtlCleaner::on_pexpire(const std::string& key, int64_t expire_at_ms, uint64_t gen) {
    std::lock_guard<std::mutex> lk(mu_);
    TimerItem item{key, expire_at_ms, gen};
    wheel_.schedule(std::move(item), kvd::util::unix_now_ms());
    cv_.notify_one();
}