#include "server/socket_io.hpp"

#include <cerrno>
#include <sys/socket.h>

bool send_all(int fd, const char* data, size_t size) {
    size_t sent = 0;
    while (sent < size) {
        ssize_t n = send(fd, data + sent, size - sent, MSG_NOSIGNAL);
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (n == 0) return false;

        sent += static_cast<size_t>(n);
    }
    return true;
}


bool send_all(int fd, const std::string& s) {
    return send_all(fd, s.data(), s.size());
}