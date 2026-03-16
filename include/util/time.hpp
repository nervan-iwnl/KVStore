#pragma once
#include <cstdint>

namespace kvd::util {

int64_t unix_now_ms();
int64_t steady_now_ms(); 

} // namespace kvd::util