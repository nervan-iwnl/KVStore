#include <mutex>
#include <unordered_map>
#include <shared_mutex>
#include <sstream>
#include <cmath>
#include <limits>
#include <type_traits>

#include "util/time.hpp"
#include "util/utils.hpp"
#include "domain/kv_store.hpp"

struct KVStore::Impl {
    struct Entry {
        std::string value;
        bool has_expire = false;
        int64_t expire_at_ms = 0;
        uint64_t ttl_gen = 0;
    };

    mutable std::shared_mutex mu_;
    std::unordered_map<std::string, Entry> map_;
    static constexpr int64_t MAX_TTL_MS = 120LL * 24 * 60 * 60 * 1000;

    bool is_expired(const Entry& e, int64_t now) const;
    void erase_if_expired(const std::string& key, int64_t now_ms);

    PexpireResult apply_expire_at_locked(
        const std::string& key,
        int64_t expire_at_ms,
        int64_t now_ms,
        std::atomic<uint64_t>& next_timer_gen
    );

    template<class T> 
    std::optional<T> incr_any(const std::string& key, T delta);  
    
    template <class T> 
    bool checked_add(T lhs, T rhs, T& out);
};


template <class T>
bool KVStore::Impl::checked_add(T lhs, T rhs, T& out) {
    if constexpr (std::is_integral_v<T>) {
        if ((rhs > 0 && lhs > std::numeric_limits<T>::max() - rhs) ||
            (rhs < 0 && lhs < std::numeric_limits<T>::min() - rhs)) {
            return false;
        }
        out = lhs + rhs;
        return true;
    } else if constexpr (std::is_floating_point_v<T>) {
        out = lhs + rhs;
        return std::isfinite(out);
    }
    return false;
}


KVStore::KVStore()
    : p_(std::make_unique<Impl>())
{}

KVStore::~KVStore() = default;

bool KVStore::Impl::is_expired(const Entry& e, int64_t now) const {
    return (e.has_expire && e.expire_at_ms <= now);
}

template<class T> 
std::optional<T> KVStore::Impl::incr_any(const std::string& key, T delta) {
    T res = 0;
    auto it = map_.find(key);
    auto now = kvd::util::unix_now_ms();
    if (it != map_.end() && !is_expired(it->second, now)) {
        if (!parse_strict(it->second.value, res)) return std::nullopt;
    } else  {
        if (it != map_.end()) map_.erase(it);
        it = map_.emplace(key, Entry{}).first;
    }
    T next = 0;
    if (!checked_add(res, delta, next)) return std::nullopt;

    res = next;
    std::ostringstream oss;
    oss << res;
    it->second.value = oss.str();
    return res;
}


void KVStore::Impl::erase_if_expired(const std::string& key, int64_t now_ms) {
    std::unique_lock<std::shared_mutex> lk(mu_);
    auto it = map_.find(key);
    if (it == map_.end()) return;
    if (is_expired(it->second, now_ms)) {
        map_.erase(it);
    }
}

PexpireResult KVStore::Impl::apply_expire_at_locked(
    const std::string& key,
    int64_t expire_at_ms,
    int64_t now_ms,
    std::atomic<uint64_t>& next_timer_gen
) {
    PexpireResult res{};
    auto it = map_.find(key);

    if (it == map_.end()) {
        res.state = PexpireResult::State::NoKey;
        return res;
    }

    if (is_expired(it->second, now_ms)) {
        map_.erase(it);
        res.state = PexpireResult::State::NoKey;
        return res;
    }

    if (expire_at_ms <= now_ms) {
        map_.erase(it);
        res.state = PexpireResult::State::DeletedImmediately;
        return res;
    }

    const int64_t ttl_ms = expire_at_ms - now_ms;
    if (ttl_ms > MAX_TTL_MS) {
        res.state = PexpireResult::State::InvalidTtl;
        return res;
    }

    it->second.has_expire = true;
    it->second.expire_at_ms = expire_at_ms;
    it->second.ttl_gen = next_timer_gen.fetch_add(1, std::memory_order_relaxed);

    res.state = PexpireResult::State::Scheduled;
    res.expire_at_ms = expire_at_ms;
    res.gen = it->second.ttl_gen;
    return res;
}

void KVStore::set(std::string key, std::string value) {
    std::unique_lock<std::shared_mutex> lk(p_->mu_);
    Impl::Entry& e = p_->map_[std::move(key)];
    e.value = std::move(value);
    e.has_expire = false;
    e.expire_at_ms = 0;

    uint64_t gen = next_timer_gen_.fetch_add(1, std::memory_order_relaxed);
    e.ttl_gen = gen;
}

std::optional<std::string> KVStore::get(const std::string& key) {
    std::shared_lock<std::shared_mutex> lk(p_->mu_);
    auto now = kvd::util::unix_now_ms();
    auto it = p_->map_.find(key);
    if (it != p_->map_.end()) {
        if (!p_->is_expired(it->second, now)) return it->second.value;
        lk.unlock();
        p_->erase_if_expired(key, now);
    }   
    return std::nullopt;
}


size_t KVStore::size() const {
    std::shared_lock<std::shared_mutex> lk(p_->mu_);
    return p_->map_.size(); 
}


bool KVStore::del(const std::string& key) {
    std::unique_lock<std::shared_mutex> lk(p_->mu_);
    auto it = p_->map_.find(key);
    if (it == p_->map_.end()) return false;
    auto now = kvd::util::unix_now_ms();
    bool is_exp = p_->is_expired(it->second, now);
    p_->map_.erase(it);
    return !is_exp;
}

std::optional<long long> KVStore::incr(const std::string& key) {
    return KVStore::incrby(key, 1);
}


std::optional<long long> KVStore::incrby(const std::string& key, long long delta) {
    std::unique_lock<std::shared_mutex> lk(p_->mu_);
    return p_->incr_any(key, delta);
}


std::optional<double> KVStore::incrbyfloat(const std::string& key, double delta) {
    std::unique_lock<std::shared_mutex> lk(p_->mu_);
    return p_->incr_any(key, delta);
}


PexpireResult KVStore::pexpire(const std::string& key, int64_t ttl_ms) {
    int64_t now = kvd::util::unix_now_ms();
    std::unique_lock<std::shared_mutex> lk(p_->mu_); 
    PexpireResult res = {};
    
    if (ttl_ms > Impl::MAX_TTL_MS) {
        res.state = PexpireResult::State::InvalidTtl;
        return res;
    }

    if (ttl_ms > 0 && now > std::numeric_limits<int64_t>::max() - ttl_ms) {
        res.state = PexpireResult::State::InvalidTtl;
        return res;
    }

    const int64_t expire_at_ms = now + ttl_ms;
    return p_->apply_expire_at_locked(key, expire_at_ms, now, next_timer_gen_);
}

PexpireResult KVStore::pexpireat(const std::string& key, int64_t expire_at_ms) {
    const int64_t now = kvd::util::unix_now_ms();
    std::unique_lock<std::shared_mutex> lk(p_->mu_);
    return p_->apply_expire_at_locked(key, expire_at_ms, now, next_timer_gen_);
}

PttlResult KVStore::pttl(const std::string& key) {
    int64_t now = kvd::util::unix_now_ms();
    std::shared_lock<std::shared_mutex> lk(p_->mu_);
    auto it = p_->map_.find(key);
    PttlResult res = {};
    res.state = PttlResult::State::NoKey;
    if (it == p_->map_.end()) return res;
    if (!it->second.has_expire) {
        res.state = PttlResult::State::NoExpire;
        return res;
    }
    if (!p_->is_expired(it->second, now)) {
        res.state = PttlResult::State::HasExpire;
        res.remaining_ms = it->second.expire_at_ms - now;
        return res;
    }
    lk.unlock();
    p_->erase_if_expired(key, now);
    return res;
}

PersistResult KVStore::persist(const std::string& key) {
    const uint64_t now = kvd::util::unix_now_ms();
    std::unique_lock<std::shared_mutex> lk(p_->mu_);

    PersistResult res{};
    auto it = p_->map_.find(key);
    if (it == p_->map_.end()) {
        res.state = PersistResult::State::NoKey;
        return res;
    }
    if (p_->is_expired(it->second, now)) {
        p_->map_.erase(it);
        res.state = PersistResult::State::NoKey;
        return res;
    }

    if (!it->second.has_expire) {
        res.state = PersistResult::State::NoExpire;
        return res;
    }

    it->second.has_expire = false;
    it->second.expire_at_ms = 0;
    it->second.ttl_gen = next_timer_gen_.fetch_add(1, std::memory_order_relaxed);

    res.state = PersistResult::State::Persisted;
    return res;
}

bool KVStore::erase_expired_if_match(const std::string& key, int64_t expected_expire_at_ms, 
        uint64_t expected_gen, int64_t now_ms) {
    std::unique_lock<std::shared_mutex> lk(p_->mu_);
    auto it = p_->map_.find(key);
    if (it == p_->map_.end()) return false;
    if (it->second.ttl_gen != expected_gen || it->second.expire_at_ms != expected_expire_at_ms ||
        it->second.expire_at_ms > now_ms || !it->second.has_expire) return false;
    p_->map_.erase(it);
    return true;
}