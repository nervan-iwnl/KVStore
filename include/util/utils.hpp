#pragma once
#include <cstdint>
#include <string>

bool parse_strict(const std::string& s, long long& out);
bool parse_strict(const std::string& s, double& out);

bool is_pow2(uint32_t x);
