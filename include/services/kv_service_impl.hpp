#pragma once

#include "runtime/status.hpp"
#include "runtime/request_context.hpp"
#include "kvd/api/v1/kv.pb.h"
#include "kvd/api/v1/kv.dispatch.gen.hpp"

namespace services {

class KvServiceImpl : public kvd::gen::KvService {
public:
    kvd::runtime::Status Get(const ::kvd::api::v1::GetRequest& req,
                             ::kvd::api::v1::GetResponse& resp,
                             kvd::runtime::RequestContext& ctx);

    kvd::runtime::Status Set(const ::kvd::api::v1::SetRequest& req,
                             ::kvd::api::v1::SetResponse& resp,
                             kvd::runtime::RequestContext& ctx);

    kvd::runtime::Status Del(const ::kvd::api::v1::DelRequest& req,
                             ::kvd::api::v1::DelResponse& resp,
                             kvd::runtime::RequestContext& ctx);

    kvd::runtime::Status Exists(const ::kvd::api::v1::ExistsRequest& req,
                                ::kvd::api::v1::ExistsResponse& resp,
                                kvd::runtime::RequestContext& ctx);

    kvd::runtime::Status Size(const ::kvd::api::v1::SizeRequest& req,
                              ::kvd::api::v1::SizeResponse& resp,
                              kvd::runtime::RequestContext& ctx);
};

} // namespace services