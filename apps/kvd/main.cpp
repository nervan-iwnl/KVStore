#include <csignal>

#include "app/context.hpp"
#include "domain/kv_store.hpp"
#include "server/server.hpp"
#include "transport/dispatcher.hpp"
#include "ttl/cleaner.hpp"

#include "services/kv_service_impl.hpp"
#include "services/numeric_service_impl.hpp"
#include "services/ttl_service_impl.hpp"

#include "kvd/api/v1/kv.dispatch.gen.hpp"
#include "kvd/api/v1/numeric.dispatch.gen.hpp"
#include "kvd/api/v1/ttl.dispatch.gen.hpp"

int main() {
    std::signal(SIGPIPE, SIG_IGN);

    KVStore store;
    TtlCleaner cleaner(store, {10, {1024, 1024, 1024}});

    AppConfig cfg;
    AppContext app{store, cleaner, cfg};

    kvd::transport::Dispatcher dispatcher;

    services::KvServiceImpl kv_service;
    kvd::gen::RegisterKvService(dispatcher, kv_service);

    services::NumericImpl numeric_service;
    kvd::gen::RegisterNumericService(dispatcher, numeric_service);
    
    services::TtlServiceImpl ttl_service;
    kvd::gen::RegisterTtlService(dispatcher, ttl_service);

    Server::Config scfg;
    scfg.port = 7777; // или другое значение
    scfg.listen_backlog = 64;
    scfg.max_connections = 256;

    cleaner.start();

    try {
        Server server(app, dispatcher, scfg);
        server.run();
    } catch (...) {
        cleaner.stop();
        throw;
    }

    cleaner.stop();
    return 0;
}