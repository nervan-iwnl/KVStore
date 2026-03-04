#include <sys/socket.h>
#include <cstdint>

#include "server/session.hpp"
#include "server/socket_io.hpp"
#include "transport/kvn1/codec.hpp"
#include "transport/kvn1/adapter.hpp"
#include "app/handler.hpp"


void handle_client_session(int cfd, AppContext& ctx) {
    Conn con;
    con.fd = cfd;

    char buf[4096];
    while (true) {
        ssize_t n = recv(cfd, buf, sizeof(buf), 0);
        if (n == 0) break;  
        if (n < 0) break;   

        con.inbuf.append(buf, static_cast<size_t>(n));

        uint8_t msg_type = 0;
        std::string payload;
        while (true) {
            auto st = kvn1::try_parse_frame(con.inbuf, msg_type, payload);
            if (st == kvn1::ParseFrameStatus::Ok) {
                CoreReq cr;
                if (!decode_kvn1_request(msg_type, payload, cr)) {
                    encode_kvn1_response({RespKind::Err, std::string("ERR protocol error")}, con.outbuf);
                    send_all(cfd, con.outbuf);
                    con.outbuf.clear();
                    return;
                }
                encode_kvn1_response(handle_core(cr, ctx), con.outbuf);
                continue;
            }
            if (st == kvn1::ParseFrameStatus::BadFrame) {
                encode_kvn1_response({RespKind::Err, std::string("ERR bad frame")}, con.outbuf);
                send_all(cfd, con.outbuf);
                con.outbuf.clear();
                return;
            }
            break;
        }

        if (!con.outbuf.empty()) {
            if (!send_all(cfd, con.outbuf)) break;
            con.outbuf.clear();
        }

        if (con.inbuf.size() > 64ull * 1024 * 1024) break;
    }
}


