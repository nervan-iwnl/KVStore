#pragma once

#include "app/command.hpp" 

struct AppContext;

CoreResp handle_pexpire(const CoreReq&, AppContext&);
CoreResp handle_pttl(const CoreReq&, AppContext&);