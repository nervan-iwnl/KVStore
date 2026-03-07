#include "ttl/cleaner.hpp"
#include "ttl/wheel.hpp" 
#include "util/utils.hpp" 


TtlCleaner::TtlCleaner(ITtlTarget& target, HierTtlWheel::Config wheel_cfg)
: target_(target)
, wheel_(std::move(wheel_cfg)) 
{}

void TtlCleaner::run() {
    std::unique_lock<std::mutex> lk(mu_);
    const int64_t tick = wheel_.tick_ms();
    int64_t next = now_ms() + tick;
    std::vector<TimerItem> due;
    while (!stop_flag_.load()) {
        int64_t now = now_ms();
        auto sleep_ms = (next - now > 0) ? (next - now) : 0;
        cv_.wait_for(lk, std::chrono::milliseconds(sleep_ms));
        if (stop_flag_.load()) break;
        now = now_ms();
        if (now < next) continue;

        const size_t steps = static_cast<size_t>((now - next) / tick + 1);

        due.clear();
        wheel_.tick_n(steps, next, due);
        next += static_cast<int64_t>(steps) * tick;

        lk.unlock();
        std::vector<TimerItem> backWheel;
        backWheel.reserve(due.size());
        for (auto &it : due) {
            if (it.expire_at_ms <= now) target_.erase_expired_if_match(it.key, it.expire_at_ms, it.gen, now);
            else backWheel.push_back(std::move(it));
        }
        lk.lock();  

        for (auto &it : backWheel) wheel_.schedule(std::move(it), now);
    }
}

void TtlCleaner::start() {
    stop_flag_ = false; 
    if (worker_.joinable()) return;
    worker_ = std::thread(&TtlCleaner::run, this);
}

void TtlCleaner::stop() {
    stop_flag_ = true;
    cv_.notify_one();
    if (worker_.joinable()) worker_.join(); 
}


void TtlCleaner::on_pexpire(const std::string& key, int64_t expire_at_ms, uint64_t gen) {
    std::lock_guard<std::mutex> lk(mu_);
    TimerItem el = {key, expire_at_ms, gen};
    wheel_.schedule(el, now_ms());
    cv_.notify_one();
}