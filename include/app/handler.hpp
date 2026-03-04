#pragma once
#include "app/dispatch.hpp"

inline CoreResp handle_core(const CoreReq& req, AppContext& ctx) {
    return dispatch(req, ctx);
}
