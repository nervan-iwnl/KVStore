#include "app/handlers/handlers_kv.hpp"
#include "app/response.hpp"
#include "domain/kv_store.hpp"
#include "app/context.hpp"
#include <string>

CoreResp handle_set(const CoreReq& req, AppContext& ctx) {
    if (req.key.empty()) return err("SET needs key");

    if (auto* s = std::get_if<std::string>(&req.arg)) {
        ctx.store.set(req.key, *s);
        return ok();
    }
    if (auto* i = std::get_if<int64_t>(&req.arg)) {
        ctx.store.set(req.key, std::to_string(*i));
        return ok();
    }
    if (auto* d = std::get_if<double>(&req.arg)) {
        ctx.store.set(req.key, std::to_string(*d));
        return ok();
    }
    return err("SET needs value");
}

CoreResp handle_get(const CoreReq& req, AppContext& ctx) {
    if (req.key.empty()) return err("GET needs key");
    auto got = ctx.store.get(req.key);
    if (!got) return nul();
    return val(*got);
}

CoreResp handle_del(const CoreReq& req, AppContext& ctx) {
    if (req.key.empty()) return err("DEL needs key");
    bool removed = ctx.store.del(req.key);
    return i64(removed ? 1 : 0);
}

CoreResp handle_size(const CoreReq&, AppContext& ctx) {
    return i64((int64_t)ctx.store.size());
}
