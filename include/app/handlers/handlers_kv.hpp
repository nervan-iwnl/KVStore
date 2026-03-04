#pragma once

#include "app/command.hpp" 

struct AppContext;

CoreResp handle_get(const CoreReq&, AppContext&);
CoreResp handle_set(const CoreReq&, AppContext&);
CoreResp handle_del(const CoreReq&, AppContext&);
CoreResp handle_size(const CoreReq&, AppContext&);