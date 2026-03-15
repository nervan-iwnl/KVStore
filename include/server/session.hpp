#pragma once

#include "app/context.hpp"
#include "transport/dispatcher.hpp"

void handle_client_session(
    int fd,
    const kvd::transport::Dispatcher& dispatcher,
    AppContext& app
);