#include "services/numeric_service_impl.hpp"
#include "app/context.hpp"
#include "domain/kv_store.hpp"

namespace services {


namespace {

kvd::runtime::Status apply_numeric_delta(
    const std::string& key,
    ::kvd::api::v1::NumericKind kind,
    std::int64_t int_delta,
    double float_delta,
    ::kvd::api::v1::NumericValueResponse& resp,
    kvd::runtime::RequestContext& ctx
) {
    if (key.empty()) {
        return kvd::runtime::bad_request("numeric op needs key");
    }

    switch (kind) {
        case ::kvd::api::v1::NUMERIC_KIND_INT: {
            auto got = ctx.app.store.incrby(key, int_delta);
            if (!got) {
                return kvd::runtime::bad_request("Value is not an integer");
            }
            resp.set_int_value(*got);
            break;
        }

        case ::kvd::api::v1::NUMERIC_KIND_FLOAT: {
            auto got = ctx.app.store.incrbyfloat(key, float_delta);
            if (!got) {
                return kvd::runtime::bad_request("Value is not a float");
            }
            resp.set_float_value(*got);
            break;
        }

        case ::kvd::api::v1::NUMERIC_KIND_UNSPECIFIED:
        default:
            return kvd::runtime::bad_request("numeric kind is required");
    }
    return kvd::runtime::ok();
}
} // namespace

kvd::runtime::Status NumericImpl::Incr(
        const ::kvd::api::v1::NumericKeyRequest& req,
        ::kvd::api::v1::NumericValueResponse& resp,
        kvd::runtime::RequestContext& ctx
    ) {

    return apply_numeric_delta(
        req.key(),
        req.kind(),
        1,
        1.0,
        resp, 
        ctx
    );
}

kvd::runtime::Status NumericImpl::Decr(
        const ::kvd::api::v1::NumericKeyRequest& req,
        ::kvd::api::v1::NumericValueResponse& resp,
        kvd::runtime::RequestContext& ctx
    ) {

    return apply_numeric_delta(
        req.key(),
        req.kind(),
        -1,
        -1.0,
        resp, 
        ctx
    );
}

kvd::runtime::Status NumericImpl::IncrBy(
    const ::kvd::api::v1::NumericDeltaRequest& req,
    ::kvd::api::v1::NumericValueResponse& resp,
    kvd::runtime::RequestContext& ctx
) {
    if (req.key().empty()) {
        return kvd::runtime::bad_request("numeric op needs key");
    }

    switch (req.delta_case()) {
        case ::kvd::api::v1::NumericDeltaRequest::kIntDelta:
            return apply_numeric_delta(
                req.key(),
                ::kvd::api::v1::NUMERIC_KIND_INT,
                req.int_delta(),
                0.0,
                resp,
                ctx
            );

        case ::kvd::api::v1::NumericDeltaRequest::kFloatDelta:
            return apply_numeric_delta(
                req.key(),
                ::kvd::api::v1::NUMERIC_KIND_FLOAT,
                0,
                req.float_delta(),
                resp,
                ctx
            );

        case ::kvd::api::v1::NumericDeltaRequest::DELTA_NOT_SET:
        default:
            return kvd::runtime::bad_request("delta is required");
    }
}

kvd::runtime::Status NumericImpl::DecrBy(
    const ::kvd::api::v1::NumericDeltaRequest& req,
    ::kvd::api::v1::NumericValueResponse& resp,
    kvd::runtime::RequestContext& ctx
) {
    if (req.key().empty()) {
        return kvd::runtime::bad_request("numeric op needs key");
    }

    switch (req.delta_case()) {
        case ::kvd::api::v1::NumericDeltaRequest::kIntDelta:
            return apply_numeric_delta(
                req.key(),
                ::kvd::api::v1::NUMERIC_KIND_INT,
                -req.int_delta(),
                0.0,
                resp,
                ctx
            );

        case ::kvd::api::v1::NumericDeltaRequest::kFloatDelta:
            return apply_numeric_delta(
                req.key(),
                ::kvd::api::v1::NUMERIC_KIND_FLOAT,
                0,
                -req.float_delta(),
                resp,
                ctx
            );

        case ::kvd::api::v1::NumericDeltaRequest::DELTA_NOT_SET:
        default:
            return kvd::runtime::bad_request("delta is required");
    }
}

} // namespace