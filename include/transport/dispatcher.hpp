#pragma once

#include "kvd/api/v1/envelope.pb.h"
#include "runtime/request_context.hpp"

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <utility>

namespace kvd::transport {

using Handler = std::function<void(
    const ::kvd::api::v1::RequestFrame&,
    ::kvd::api::v1::ResponseFrame&,
    ::kvd::runtime::RequestContext&
)>;

class Dispatcher {
public:
    void Register(std::uint32_t service_id,
                  std::uint32_t method_id,
                  Handler handler) {
        handlers_[make_key(service_id, method_id)] = std::move(handler);
    }

    const Handler* Find(std::uint32_t service_id,
                        std::uint32_t method_id) const {
        auto it = handlers_.find(make_key(service_id, method_id));
        if (it == handlers_.end()) {
            return nullptr;
        }
        return &it->second;
    }

    bool Dispatch(const ::kvd::api::v1::RequestFrame& req,
                  ::kvd::api::v1::ResponseFrame& resp,
                  ::kvd::runtime::RequestContext& ctx) const {
        const Handler* h = Find(req.service_id(), req.method_id());
        if (h == nullptr) {
            return false;
        }
        (*h)(req, resp, ctx);
        return true;
    }

private:
    static std::uint64_t make_key(std::uint32_t service_id,
                                  std::uint32_t method_id) {
        return (static_cast<std::uint64_t>(service_id) << 32) |
               static_cast<std::uint64_t>(method_id);
    }

    std::unordered_map<std::uint64_t, Handler> handlers_;
};

} // namespace kvd::transport