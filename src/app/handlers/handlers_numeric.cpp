#include "app/handlers/handlers_numeric.hpp"
#include "app/response.hpp"
#include "app/context.hpp"
#include "domain/kv_store.hpp"

CoreResp handle_incr(const CoreReq& req, AppContext& ctx) {
    if (req.key.empty()) return err("INCR needs key");
    auto r = ctx.store.incr(req.key);
    if (!r) return err("NOT A NUMBER");
    return i64(*r);
}

CoreResp handle_incrby(const CoreReq& req, AppContext& ctx) {
    if (req.key.empty()) return err("INCRBY needs key");
    auto* d = std::get_if<int64_t>(&req.arg);
    if (!d) return err("INCRBY needs int64 delta");
    auto r = ctx.store.incrby(req.key, *d);
    if (!r) return err("NOT A NUMBER");
    return i64(*r);
}

CoreResp handle_incrbyfloat(const CoreReq& req, AppContext& ctx) {
    if (req.key.empty()) return err("INCRBYFLOAT needs key");
    auto* d = std::get_if<double>(&req.arg);
    if (!d) return err("INCRBYFLOAT needs double delta");
    auto r = ctx.store.incrbyfloat(req.key, *d);
    if (!r) return err("NOT A NUMBER");
    return f64(*r);
}
