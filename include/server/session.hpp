#pragma once

#include "app/context.hpp"

#include <string>


struct Conn {
    int fd;
    std::string inbuf;
    std::string outbuf;
};

void handle_client_session(int fd, AppContext& ctx);