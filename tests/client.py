import socket
import struct
import time
from dataclasses import dataclass
from typing import Optional, Tuple, List, Union

MAGIC = b"KVN1"

# ====== EDIT THESE TO MATCH YOUR kvf1.hpp ======

# MsgType (request frame type byte)
MSG_PING    = 0x01
MSG_GET     = 0x02
MSG_SET     = 0x03
MSG_DEL     = 0x04
MSG_SIZE    = 0x05
MSG_INCR    = 0x06
MSG_INCRBY  = 0x07
MSG_INCRBYF = 0x08
MSG_PEXPIRE = 0x09  # <-- choose your value
MSG_PTTL    = 0x0A  # <-- choose your value
MSG_INFO     = 0x0B
MSG_CAPS     = 0x0C
MSG_COMMANDS = 0x0D

# FieldType (TLV inside payload)
FT_KEY   = 0x01
FT_VALUE = 0x02
FT_I64   = 0x03
FT_F64   = 0x04

# RespType (response frame type byte)
RESP_OK    = 0x81
RESP_ERR   = 0x82
RESP_NULL  = 0x86
RESP_VALUE = 0x83
RESP_INT   = 0x84
RESP_FLOAT = 0x85

# =============================================

def tlv_bytes(ft: int, b: bytes) -> bytes:
    return struct.pack("!BI", ft, len(b)) + b

def tlv_str(ft: int, s: str) -> bytes:
    return tlv_bytes(ft, s.encode("utf-8"))

def tlv_i64(x: int) -> bytes:
    return struct.pack("!BIq", FT_I64, 8, x)

def tlv_f64(x: float) -> bytes:
    return struct.pack("!BI", FT_F64, 8) + struct.pack("!d", x)

def frame(msg_type: int, payload: bytes) -> bytes:
    # LEN = 1(type) + payload
    ln = 1 + len(payload)
    return MAGIC + struct.pack("!I", ln) + bytes([msg_type]) + payload

def recv_exact(sock: socket.socket, n: int) -> bytes:
    chunks = []
    got = 0
    while got < n:
        part = sock.recv(n - got)
        if not part:
            raise ConnectionError("socket closed")
        chunks.append(part)
        got += len(part)
    return b"".join(chunks)

def recv_frame(sock: socket.socket) -> Tuple[int, bytes]:
    hdr = recv_exact(sock, 8)  # MAGIC(4) + LEN(4)
    if hdr[:4] != MAGIC:
        raise ValueError(f"bad MAGIC: {hdr[:4]!r}")
    ln = struct.unpack("!I", hdr[4:8])[0]
    body = recv_exact(sock, ln)
    rtype = body[0]
    payload = body[1:]
    return rtype, payload

def tlv_iter(payload: bytes):
    i = 0
    n = len(payload)
    while i + 5 <= n:
        ft = payload[i]
        ln = struct.unpack("!I", payload[i+1:i+5])[0]
        i += 5
        if i + ln > n:
            raise ValueError("bad TLV length")
        v = payload[i:i+ln]
        i += ln
        yield ft, v
    if i != n:
        raise ValueError("trailing bytes in TLV payload")

def resp_to_str(rtype: int, payload: bytes) -> str:
    # decode common “Value” field if present
    fields = list(tlv_iter(payload)) if payload else []
    def get_value_bytes() -> Optional[bytes]:
        for ft, v in fields:
            if ft == FT_VALUE:
                return v
        return None

    if rtype == RESP_OK:
        return "OK"
    if rtype == RESP_NULL:
        return "NULL"
    if rtype == RESP_ERR:
        vb = get_value_bytes()
        msg = vb.decode("utf-8", errors="replace") if vb else "ERR"
        return f"ERR {msg}"
    if rtype == RESP_VALUE:
        vb = get_value_bytes()
        if vb is None:
            return "VALUE <missing>"
        return f"VALUE {vb.decode('utf-8', errors='replace')}"
    if rtype == RESP_INT:
        # expect FT_I64 somewhere
        for ft, v in fields:
            if ft == FT_I64 and len(v) == 8:
                x = struct.unpack("!q", v)[0]
                return f"INT {x}"
        return "INT <missing>"
    if rtype == RESP_FLOAT:
        for ft, v in fields:
            if ft == FT_F64 and len(v) == 8:
                x = struct.unpack("!d", v)[0]
                return f"FLOAT {x}"
        return "FLOAT <missing>"
    return f"RESP({rtype}) payload_len={len(payload)}"

class KVClient:
    def __init__(self, host="127.0.0.1", port=7777, timeout=5.0):
        self.sock = socket.create_connection((host, port), timeout=timeout)
        self.sock.settimeout(timeout)

    def close(self):
        try:
            self.sock.close()
        except Exception:
            pass

    def send_req(self, msg_type: int, payload: bytes) -> Tuple[int, bytes]:
        self.sock.sendall(frame(msg_type, payload))
        return recv_frame(self.sock)

    # ===== commands =====

    def ping(self):
        return self.send_req(MSG_PING, b"")

    def set(self, key: str, value: Union[str, int, float, bytes]):
        p = tlv_str(FT_KEY, key)
        if isinstance(value, bytes):
            p += tlv_bytes(FT_VALUE, value)
        elif isinstance(value, str):
            p += tlv_str(FT_VALUE, value)
        elif isinstance(value, int):
            p += tlv_i64(value)
        elif isinstance(value, float):
            p += tlv_f64(value)
        else:
            raise TypeError("bad value type")
        return self.send_req(MSG_SET, p)

    def get(self, key: str):
        p = tlv_str(FT_KEY, key)
        return self.send_req(MSG_GET, p)

    def delete(self, key: str):
        p = tlv_str(FT_KEY, key)
        return self.send_req(MSG_DEL, p)

    def pexpire(self, key: str, ttl_ms: int):
        p = tlv_str(FT_KEY, key) + tlv_i64(ttl_ms)
        return self.send_req(MSG_PEXPIRE, p)

    def pttl(self, key: str):
        p = tlv_str(FT_KEY, key)
        return self.send_req(MSG_PTTL, p)
    
    def info(self):
        return self.send_req(MSG_INFO, b"")

    def caps(self):
        return self.send_req(MSG_CAPS, b"")

    def commands(self):
        return self.send_req(MSG_COMMANDS, b"")




def main():
    c = KVClient("127.0.0.1", 7777, timeout=5.0)
    try:
        for (name, (rt, pl)) in [
            ("PING", c.ping()),
            ("SET a x", c.set("a", "x")),
            ("PTTL a", c.pttl("a")),
            ("PEXPIRE a 1200", c.pexpire("a", 1200)),
            ("PTTL a", c.pttl("a")),
            ("GET a", c.get("a")),
            ("INFO", c.info()),
            ("CAPS", c.caps()),
            ("COMMANDS", c.commands()),
        ]:
            print(f"{name:14} -> {resp_to_str(rt, pl)}")

        time.sleep(1.3)
        rt, pl = c.get("a")
        print(f"{'GET a':14} -> {resp_to_str(rt, pl)}")
        rt, pl = c.pttl("a")
        print(f"{'PTTL a':14} -> {resp_to_str(rt, pl)}")
    finally:
        c.close()

if __name__ == "__main__":
    main()
