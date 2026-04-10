#pragma once

#include <atomic>

#include "app/context.hpp"
#include "config/app_config.hpp"
#include "transport/dispatcher.hpp"

class Server {
public:
    using Config = kvd::config::ListenerConfig;

    Server(AppContext& ctx,
           const kvd::transport::Dispatcher& dispatcher,
           Config cfg);

    Server(AppContext& ctx,
           const kvd::transport::Dispatcher& dispatcher,
           const kvd::config::AppConfig& app_cfg)
        : Server(ctx, dispatcher, app_cfg.listener()) {}

    Server(AppContext& ctx,
           const kvd::transport::Dispatcher& dispatcher,
           const kvd::config::AppConfigSpec& spec)
        : Server(ctx, dispatcher, spec.listener) {}

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