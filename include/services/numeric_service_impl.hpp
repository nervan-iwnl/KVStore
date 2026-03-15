#pragma once

#include "kvd/api/v1/numeric.dispatch.gen.hpp"
#include "domain/kv_store.hpp"

namespace services {

class NumericImpl final : public kvd::gen::NumericService {
public:

    kvd::runtime::Status Incr(
        const ::kvd::api::v1::NumericKeyRequest& req,
        ::kvd::api::v1::NumericValueResponse& resp,
        kvd::runtime::RequestContext& ctx
    ) override;

    kvd::runtime::Status Decr(
        const ::kvd::api::v1::NumericKeyRequest& req,
        ::kvd::api::v1::NumericValueResponse& resp,
        kvd::runtime::RequestContext& ctx
    ) override;

    kvd::runtime::Status IncrBy(
        const ::kvd::api::v1::NumericDeltaRequest& req,
        ::kvd::api::v1::NumericValueResponse& resp,
        kvd::runtime::RequestContext& ctx
    ) override;

    kvd::runtime::Status DecrBy(
        const ::kvd::api::v1::NumericDeltaRequest& req,
        ::kvd::api::v1::NumericValueResponse& resp,
        kvd::runtime::RequestContext& ctx
    ) override;
};
} // namespace kvd::services