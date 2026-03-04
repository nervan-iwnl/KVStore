#pragma once
#include "command.hpp"

class AppContext;

CoreResp dispatch(const CoreReq& req, AppContext& ctx);
