#include "services/ttl_service_impl.hpp"
#include "app/context.hpp"
#include "domain/kv_store.hpp"
#include "ttl/cleaner.hpp"

namespace services {

namespace {

kvd::runtime::Status fill_expire_response(
    const std::string& key,
    const PexpireResult& res,
    ::kvd::api::v1::PExpireResponse& resp,
    kvd::runtime::RequestContext& ctx
) {
    switch (res.state) {
        case PexpireResult::State::NoKey:
            resp.set_state(::kvd::api::v1::PEXPIRE_STATE_NO_KEY);
            return kvd::runtime::ok();

        case PexpireResult::State::InvalidTtl:
            return kvd::runtime::bad_request("invalid ttl/expire_at_ms");

        case PexpireResult::State::DeletedImmediately:
            resp.set_state(::kvd::api::v1::PEXPIRE_STATE_DELETED_IMMEDIATELY);
            resp.set_expire_at_ms(0);
            return kvd::runtime::ok();

        case PexpireResult::State::Scheduled:
            ctx.app.cleaner.on_pexpire(key, res.expire_at_ms, res.gen);
            resp.set_state(::kvd::api::v1::PEXPIRE_STATE_SCHEDULED);
            resp.set_expire_at_ms(res.expire_at_ms);
            return kvd::runtime::ok();
    }

    return kvd::runtime::internal_error("unexpected expire state");
}

} // namespace

kvd::runtime::Status TtlServiceImpl::PExpire(
        const ::kvd::api::v1::PExpireRequest& req,
        ::kvd::api::v1::PExpireResponse& resp,
        kvd::runtime::RequestContext& ctx
) {
    if (req.key().empty()) {
        return kvd::runtime::bad_request("PEXPIRE needs key");
    }
    PexpireResult res = ctx.app.store.pexpire(req.key(), req.ttl_ms());
    return fill_expire_response(req.key(), res, resp, ctx);
}

kvd::runtime::Status TtlServiceImpl::PExpireAt(
    const ::kvd::api::v1::PExpireAtRequest& req,
    ::kvd::api::v1::PExpireResponse& resp,
    kvd::runtime::RequestContext& ctx
) {
    if (req.key().empty()) {
        return kvd::runtime::bad_request("PEXPIREAT needs key");
    }
    const auto res = ctx.app.store.pexpireat(req.key(), req.expire_at_ms());
    return fill_expire_response(req.key(), res, resp, ctx);
}

kvd::runtime::Status TtlServiceImpl::PTtl(
        const ::kvd::api::v1::TtlKeyRequest& req,
        ::kvd::api::v1::PTtlResponse& resp,
        kvd::runtime::RequestContext& ctx
) {
    if (req.key().empty()) {
        return kvd::runtime::bad_request("PTTL needs key");
    }

    const auto res = ctx.app.store.pttl(req.key());

    switch (res.state) {
        case PttlResult::State::NoKey:
            resp.set_state(::kvd::api::v1::PTTL_STATE_NO_KEY);
            resp.set_remaining_ms(0);
            return kvd::runtime::ok();

        case PttlResult::State::NoExpire:
            resp.set_state(::kvd::api::v1::PTTL_STATE_NO_EXPIRE);
            resp.set_remaining_ms(0);
            return kvd::runtime::ok();

        case PttlResult::State::HasExpire:
            resp.set_state(::kvd::api::v1::PTTL_STATE_HAS_EXPIRE);
            resp.set_remaining_ms(res.remaining_ms);
            return kvd::runtime::ok();
    }

    return kvd::runtime::internal_error("PTTL internal state");
}


kvd::runtime::Status TtlServiceImpl::Persist(
    const ::kvd::api::v1::TtlKeyRequest& req,
    ::kvd::api::v1::PersistResponse& resp,
    kvd::runtime::RequestContext& ctx
) {
    if (req.key().empty()) {
        return kvd::runtime::bad_request("PERSIST needs key");
    }

    const auto res = ctx.app.store.persist(req.key());

    switch (res.state) {
        case PersistResult::State::NoKey:
            resp.set_state(::kvd::api::v1::PERSIST_STATE_NO_KEY);
            return kvd::runtime::ok();
        case PersistResult::State::NoExpire:
            resp.set_state(::kvd::api::v1::PERSIST_STATE_NO_KEY);
            return kvd::runtime::ok();
        case PersistResult::State::Persisted:
            resp.set_state(::kvd::api::v1::PERSIST_STATE_PERSISTED);
            return kvd::runtime::ok();
    }
    return kvd::runtime::internal_error("PERSIST internal state");
}
} // namespace
