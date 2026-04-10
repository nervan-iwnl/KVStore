#pragma once

#include "config/app_config.hpp"

#include <string>
#include <vector>

namespace kvd::config {

struct AppConfigBootstrapOptions {
    bool apply_environment = true;
    bool validate = true;
    std::string environment_prefix = "KVD_";
};

[[nodiscard]] auto build_app_config(AppConfigBootstrapOptions options = {}) -> AppConfig;
[[nodiscard]] auto build_app_config(AppConfigBuilder builder, AppConfigBootstrapOptions options) -> AppConfig;
auto format_config_issues(const std::vector<ConfigIssue>& issues) -> std::string;

} // namespace kvd::config