#include <csignal>
#include <exception>
#include <iostream>

#include "app/context.hpp"
#include "config/config.hpp"
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

    try {
        const auto cfg = kvd::config::build_app_config();

        KVStore store;
        TtlCleaner cleaner(store, cfg);
        AppContext app{store, cleaner, cfg};

        kvd::transport::Dispatcher dispatcher;

        services::KvServiceImpl kv_service;
        kvd::gen::RegisterKvService(dispatcher, kv_service);

        services::NumericImpl numeric_service;
        kvd::gen::RegisterNumericService(dispatcher, numeric_service);

        services::TtlServiceImpl ttl_service;
        kvd::gen::RegisterTtlService(dispatcher, ttl_service);

        cleaner.start();
        struct CleanerStopper {
            TtlCleaner& cleaner;
            ~CleanerStopper() { cleaner.stop(); }
        } cleaner_stopper{cleaner};

        Server server(app, dispatcher, cfg);
        server.run();
    } catch (const kvd::config::ConfigValidationError& ex) {
        std::cerr << "Config validation failed:\n"
                  << kvd::config::format_config_issues(ex.issues());
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}