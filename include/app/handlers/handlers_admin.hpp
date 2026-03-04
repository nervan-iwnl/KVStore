#pragma once

#include "app/command.hpp" 

struct AppContext;

CoreResp handle_ping(const CoreReq&, AppContext&);

// CoreResp handle_hello(const CoreReq&, AppContext&);
CoreResp handle_info(const CoreReq&, AppContext&);

CoreResp handle_caps(const CoreReq&, AppContext&);

CoreResp handle_commands(const CoreReq&, AppContext&);