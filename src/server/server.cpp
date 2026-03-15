#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <thread>

#include "server/server.hpp"
#include "server/session.hpp"
#include "kvd/api/v1/kv.dispatch.gen.hpp"


Server::Server(AppContext& ctx,
               const kvd::transport::Dispatcher& dispatcher,
               Config cfg)
    : ctx_(ctx), dispatcher_(dispatcher), cfg_(cfg) {}
    

int Server::make_listen_socket() const {
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) {
        std::cerr << "socket() failed\n";
        return -1;
    }

    int opt = 1;
    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0) {
        std::cerr << "setsockopt() failed\n";
        close(sfd);
        return -1;
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(cfg_.port);

    if (bind(sfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        std::cerr << "bind() failed\n";
        close(sfd);
        return -1;
    }
    if (listen(sfd, cfg_.listen_backlog) != 0) {
        std::cerr << "listen() failed\n";
        close(sfd);
        return -1;
    }

    std::cout << "Listening on " << cfg_.port << "\n";
    return sfd;
}


void Server::accept_loop(int sfd) {
    sockaddr_in caddr{};
    while (true) {
        socklen_t clen = sizeof(caddr);
        int cfd = accept(sfd, reinterpret_cast<sockaddr*>(&caddr), &clen);
        if (cfd < 0) {
            std::cerr << "accept() failed\n";
            continue;
        }

        int prev = active_conns_.fetch_add(1, std::memory_order_relaxed);
        if (prev >= cfg_.max_connections) {
            active_conns_.fetch_sub(1, std::memory_order_relaxed);
            std::cerr << "Max connections detected\n";
            close(cfd);
            continue;
        }

        std::cout << "Client connected\n";
        spawn_client(cfd);
    }
}


void Server::spawn_client(int cfd) {
    try {
        std::thread([this, cfd]() {
            client_worker(cfd);
        }).detach();
    } catch (...) {
        active_conns_.fetch_sub(1, std::memory_order_relaxed);
        close(cfd);
        throw;
    }
}

void Server::client_worker(int cfd) {
    struct Guard {
        int fd;
        std::atomic<int>& active;
        ~Guard() {
            if (fd >= 0) close(fd); 
            active.fetch_sub(1, std::memory_order_relaxed);
            std::cout << "Client disconnected\n"; 
        }
    } guard{cfd, active_conns_};

    handle_client_session(cfd, dispatcher_, ctx_);
}


void Server::run() {
    int sfd = make_listen_socket();
    if (sfd < 0) return;

    struct Guard {
        int fd;
        ~Guard() { if (fd >= 0) close(fd); }
    } guard{sfd};

    accept_loop(sfd);
}

