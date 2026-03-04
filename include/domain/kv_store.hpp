#pragma once
#include <string>
#include <optional>
#include <memory>
#include <cstdint>
#include <cstddef>
#include <atomic>

#include "ttl/types.hpp"
#include "domain/types.hpp"


class KVStore: public ITtlTarget {
public:
    ~KVStore();
    KVStore();

    KVStore(const KVStore&) = delete;
    KVStore& operator=(const KVStore&) = delete;
    void set(std::string key, std::string value);

    std::optional<std::string> get(const std::string& key);

    bool del(const std::string& key);

    std::optional<long long> incr(const std::string& key);
    std::optional<long long> incrby(const std::string& key, long long delta);
    std::optional<double> incrbyfloat(const std::string& key, double delta);

    PexpireResult pexpire(const std::string& key, int64_t ttl_ms);
    PttlResult pttl(const std::string& key);

    size_t size() const;

    bool erase_expired_if_match(const std::string& key, int64_t expected_expire_at_ms, 
        uint64_t expected_gen, int64_t now_ms) override;
private:
    struct Impl;
    std::unique_ptr<Impl> p_;
    std::atomic<uint64_t> next_timer_gen_ {1};
};
