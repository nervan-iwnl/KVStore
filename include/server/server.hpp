#pragma once

#include <atomic>

#include "app/context.hpp"
#include "transport/dispatcher.hpp"

class Server {
public:
    struct Config {
        int port = 7777;
        int listen_backlog = 64;
        int max_connections = 256;
    };

    Server(AppContext& ctx,
           const kvd::transport::Dispatcher& dispatcher,
           Config cfg);

    void run();

private:
    AppContext& ctx_;
    const kvd::transport::Dispatcher& dispatcher_;
    Config cfg_;
    std::atomic<int> active_conns_{0};

    int make_listen_socket() const;
    void accept_loop(int sfd);
    void spawn_client(int cfd);
    void client_worker(int cfd);
};