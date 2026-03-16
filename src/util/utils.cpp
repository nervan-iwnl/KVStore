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


bool is_pow2(uint32_t x) {
    return x && ((x & (x - 1)) == 0);
}