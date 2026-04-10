#include "server/session.hpp"

#include "server/socket_io.hpp"
#include "runtime/dispatch_helpers.hpp"
#include "kvd/api/v1/envelope.pb.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstdint>
#include <string>
#include <sys/socket.h>

namespace {

bool recv_all(int fd, void* dst, std::size_t size) {
    auto* out = static_cast<char*>(dst);
    std::size_t got = 0;

    while (got < size) {
        ssize_t n = recv(fd, out + got, size - got, 0);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        if (n == 0) {
            return false;
        }
        got += static_cast<std::size_t>(n);
    }

    return true;
}

bool read_sized_frame(int fd, std::size_t max_frame_size, std::string& out) {
    std::uint32_t len_be = 0;
    if (!recv_all(fd, &len_be, sizeof(len_be))) {
        return false;
    }

    const std::uint32_t len = ntohl(len_be);
    if (len == 0 || len > max_frame_size) {
        return false;
    }

    out.resize(len);
    return recv_all(fd, out.data(), len);
}

bool write_sized_frame(int fd, const ::google::protobuf::Message& msg) {
    std::string body;
    if (!msg.SerializeToString(&body)) {
        return false;
    }

    const auto size = static_cast<std::uint32_t>(body.size());
    const std::uint32_t len_be = htonl(size);

    if (!send_all(fd, reinterpret_cast<const char*>(&len_be), sizeof(len_be))) {
        return false;
    }
    return send_all(fd, body);
}

} // namespace

void handle_client_session(
    int cfd,
    const kvd::transport::Dispatcher& dispatcher,
    AppContext& app
) {
    std::string frame_bytes;

    while (read_sized_frame(cfd, app.config.transport().max_frame_bytes, frame_bytes)) {
        ::kvd::api::v1::RequestFrame req;
        ::kvd::api::v1::ResponseFrame resp;

        if (!req.ParseFromString(frame_bytes)) {
            resp.set_protocol_version(1);
            resp.set_request_id(0);
            kvd::runtime::fill_error(
                resp,
                ::kvd::api::v1::ProtoError::BAD_REQUEST,
                "bad RequestFrame protobuf"
            );

            if (!write_sized_frame(cfd, resp)) {
                break;
            }
            continue;
        }

        resp.set_protocol_version(req.protocol_version());
        resp.set_request_id(req.request_id());

        kvd::runtime::RequestContext req_ctx{app};

        if (!dispatcher.Dispatch(req, resp, req_ctx)) {
            kvd::runtime::fill_error(
                resp,
                ::kvd::api::v1::ProtoError::UNKNOWN_METHOD,
                "unknown service/method"
            );
        }

        if (!write_sized_frame(cfd, resp)) {
            break;
        }
    }
}