#pragma once

#include "app/command.hpp"

struct AppContext;

CoreResp handle_incr(const CoreReq&, AppContext&);
CoreResp handle_incrby(const CoreReq&, AppContext&);
CoreResp handle_incrbyfloat(const CoreReq&, AppContext&);