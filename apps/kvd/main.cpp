#include <csignal>

#include "domain/kv_store.hpp"
#include "server/server.hpp"
#include "app/context.hpp"
#include "ttl/cleaner.hpp"



int main() {
    std::signal(SIGPIPE, SIG_IGN);

    KVStore store;
    TtlCleaner cleaner(store, {10, {1024, 1024, 1024}});
    cleaner.start();

    AppConfig cfg;
    AppContext ctx = {store, cleaner, cfg};
    
    Server::Config scfg;
    
    Server server(ctx, scfg);
    server.run();
    
    cleaner.stop();
    return 0;
}
