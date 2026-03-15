from __future__ import annotations

import socket
import struct
from itertools import count
from typing import Type, TypeVar

from .proto_runtime import envelope_pb2

T = TypeVar("T")


def recv_exact(sock: socket.socket, n: int) -> bytes:
    buf = bytearray()
    while len(buf) < n:
        chunk = sock.recv(n - len(buf))
        if not chunk:
            raise ConnectionError("socket closed by peer")
        buf.extend(chunk)
    return bytes(buf)


def send_frame(sock: socket.socket, payload: bytes) -> None:
    sock.sendall(struct.pack("!I", len(payload)))
    sock.sendall(payload)


def recv_frame(sock: socket.socket) -> bytes:
    header = recv_exact(sock, 4)
    (size,) = struct.unpack("!I", header)
    if size <= 0:
        raise ValueError(f"invalid frame size: {size}")
    return recv_exact(sock, size)


class RpcError(RuntimeError):
    def __init__(self, code: int, message: str):
        super().__init__(f"rpc error code={code}: {message}")
        self.code = code
        self.message = message


class RpcClient:
    PROTOCOL_VERSION = 1

    def __init__(self, host: str, port: int, timeout: float):
        self._sock = socket.create_connection((host, port), timeout=timeout)
        self._sock.settimeout(timeout)
        self._request_ids = count(1)

    def close(self) -> None:
        try:
            self._sock.close()
        except Exception:
            pass

    def __enter__(self) -> "RpcClient":
        return self

    def __exit__(self, exc_type, exc, tb) -> None:
        self.close()

    def _call(self, service_id: int, method_id: int, req_msg, resp_cls: Type[T]) -> T:
        req_frame = envelope_pb2.RequestFrame()
        req_frame.protocol_version = self.PROTOCOL_VERSION
        req_frame.service_id = service_id
        req_frame.method_id = method_id
        req_frame.request_id = next(self._request_ids)
        req_frame.payload = req_msg.SerializeToString()

        send_frame(self._sock, req_frame.SerializeToString())

        raw_resp = recv_frame(self._sock)
        resp_frame = envelope_pb2.ResponseFrame()
        resp_frame.ParseFromString(raw_resp)

        if resp_frame.request_id != req_frame.request_id:
            raise RuntimeError(
                f"request_id mismatch: sent={req_frame.request_id}, got={resp_frame.request_id}"
            )

        if not resp_frame.ok:
            raise RpcError(resp_frame.error.code, resp_frame.error.message)

        resp = resp_cls()
        if resp_frame.payload:
            resp.ParseFromString(resp_frame.payload)
        return resp