#pragma once

#include "config/app_config.hpp"

class KVStore;
class TtlCleaner;

struct AppContext {
    KVStore& store;
    TtlCleaner& cleaner;
    const AppConfig& config;
};
