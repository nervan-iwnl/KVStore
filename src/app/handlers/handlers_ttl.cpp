#include "app/handlers/handlers_ttl.hpp"
#include "app/response.hpp"
#include "app/context.hpp"
#include "domain/kv_store.hpp"
#include "ttl/cleaner.hpp"

CoreResp handle_pttl(const CoreReq& req, AppContext& ctx) {
    if (req.key.empty()) return err("PTTL needs key");
    auto r = ctx.store.pttl(req.key);
    switch (r.state) {
        case PttlResult::State::NoKey:     return nul();
        case PttlResult::State::NoExpire:  return ok();
        case PttlResult::State::HasExpire: return i64(r.remaining_ms);
    }
    return err("PTTL internal state");
}

CoreResp handle_pexpire(const CoreReq& req, AppContext& ctx) {
    if (req.key.empty()) return err("PEXPIRE needs key");
    auto* ms = std::get_if<int64_t>(&req.arg);
    if (!ms) return err("PEXPIRE needs int64 ttl_ms");
    PexpireResult res = ctx.store.pexpire(req.key, *ms);
        switch (res.state) {
        case PexpireResult::State::NoKey:
            return nul();

        case PexpireResult::State::InvalidTtl:
            return err("PEXPIRE invalid ttl");

        case PexpireResult::State::DeletedImmediately:
            return i64(1);

        case PexpireResult::State::Scheduled:
            ctx.cleaner.on_pexpire(req.key, res.expire_at_ms, res.gen);
            return i64(1);
    }

    return err("PEXPIRE internal state error");
}
