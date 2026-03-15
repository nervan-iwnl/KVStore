#include "services/kv_service_impl.hpp"
#include "domain/kv_store.hpp"

namespace services {

kvd::runtime::Status KvServiceImpl::Get(
    const ::kvd::api::v1::GetRequest& req,
    ::kvd::api::v1::GetResponse& resp,
    kvd::runtime::RequestContext& ctx) {

    if (req.key().empty()) {
        return kvd::runtime::bad_request("GET needs key");
    }
    auto got = ctx.app.store.get(req.key());
    if (got) {
        resp.set_value(*got);
    }

    return kvd::runtime::ok();
}

kvd::runtime::Status KvServiceImpl::Set(
    const ::kvd::api::v1::SetRequest& req,
    ::kvd::api::v1::SetResponse& resp,
    kvd::runtime::RequestContext& ctx) {
    
    if (req.key().empty()) {
        return kvd::runtime::bad_request("SET needs key");
    } 
    ctx.app.store.set(req.key(), req.value());

    resp.set_applied(true);
    return kvd::runtime::ok();    
}

kvd::runtime::Status KvServiceImpl::Del(const ::kvd::api::v1::DelRequest& req,
    ::kvd::api::v1::DelResponse& resp,
    kvd::runtime::RequestContext& ctx) {
    
    if (req.key().empty()) {
        return kvd::runtime::bad_request("DEL needs key");
    }

    const bool removed = ctx.app.store.del(req.key());
    resp.set_removed(removed);

    return kvd::runtime::ok();
}


kvd::runtime::Status KvServiceImpl::Exists(const ::kvd::api::v1::ExistsRequest& req,
    ::kvd::api::v1::ExistsResponse& resp,
    kvd::runtime::RequestContext& ctx) {

    if (req.key().empty()) {
        return kvd::runtime::bad_request("EXISTS needs key");
    }
    
    resp.set_exists(ctx.app.store.get(req.key()).has_value());
    return kvd::runtime::ok();
}

kvd::runtime::Status KvServiceImpl::Size(const ::kvd::api::v1::SizeRequest&,
    ::kvd::api::v1::SizeResponse& resp,
    kvd::runtime::RequestContext& ctx) {
    
    resp.set_size(static_cast<uint64_t>(ctx.app.store.size()));
    
    return kvd::runtime::ok();
}

} // namespace services