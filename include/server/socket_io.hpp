#pragma once

#include <string>
#include <cstddef>


bool send_all(int fd, const char* data, size_t size);

bool send_all(int fd, const std::string& s);
