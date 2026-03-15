#pragma once

#include "runtime/status.hpp"
#include "runtime/request_context.hpp"
#include "kvd/api/v1/ttl.pb.h"
#include "kvd/api/v1/ttl.dispatch.gen.hpp"

namespace services {

class TtlServiceImpl final : public kvd::gen::TtlService {
public:
    kvd::runtime::Status PExpire(
        const ::kvd::api::v1::PExpireRequest& req,
        ::kvd::api::v1::PExpireResponse& resp,
        kvd::runtime::RequestContext& ctx
    ) override;

    kvd::runtime::Status PTtl(
        const ::kvd::api::v1::TtlKeyRequest& req,
        ::kvd::api::v1::PTtlResponse& resp,
        kvd::runtime::RequestContext& ctx
    ) override;

    kvd::runtime::Status Persist(
        const ::kvd::api::v1::TtlKeyRequest& req,
        ::kvd::api::v1::PersistResponse& resp,
        kvd::runtime::RequestContext& ctx
    ) override;

    kvd::runtime::Status PExpireAt(
        const ::kvd::api::v1::PExpireAtRequest& req,
        ::kvd::api::v1::PExpireResponse& resp,
        kvd::runtime::RequestContext& ctx
    ) override;
};

} // namespace services