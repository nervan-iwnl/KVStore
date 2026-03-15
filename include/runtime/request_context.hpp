#pragma once
#include "app/context.hpp"

namespace kvd::runtime {

struct RequestContext {
    AppContext& app;
};

} // namespace kvd::runtime