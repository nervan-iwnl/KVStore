#include "util/utils.hpp"
#include <chrono>

bool parse_strict(const std::string& s, long long &out) {
    try {
        size_t pos = 0;
        out = std::stoll(s, &pos);
        return pos == s.size();
    } catch (...) {
        return false;
    }
}

bool parse_strict(const std::string& s, double &out) {
    try {
        size_t pos = 0;
        out = std::stod(s, &pos);
        return pos == s.size();
    } catch (...) {
        return false;
    }
}

int64_t now_ms() {
    auto ts = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(ts.time_since_epoch()).count();
}

bool is_pow2(uint32_t x) {
    return x && ((x & (x - 1)) == 0);
}