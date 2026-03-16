#pragma once

#include <cstdint>

struct PttlResult {
    enum class State {
        NoKey,
        NoExpire,
        HasExpire
    } state = State::NoKey;

    int64_t remaining_ms = 0; // only if HasExpire
};

struct PexpireResult {
    enum class State {
        NoKey,
        InvalidTtl,
        DeletedImmediately,
        Scheduled
    }  state = State::NoKey;
    int64_t expire_at_ms = 0;
    uint64_t gen = 0;
};

struct PersistResult {
    enum class State {
        NoKey,
        NoExpire,
        Persisted
    } state{};
};