from __future__ import annotations

import time

from .proto_runtime import kv_pb2, numeric_pb2, ttl_pb2
from .transport import RpcClient


class KvdClient(RpcClient):
    SERVICE_KV = 1
    SERVICE_NUMERIC = 2
    SERVICE_TTL = 3

    METHOD_GET = 1
    METHOD_SET = 2
    METHOD_DEL = 3
    METHOD_EXISTS = 4
    METHOD_SIZE = 5

    METHOD_NUMERIC_INCR = 1
    METHOD_NUMERIC_DECR = 2
    METHOD_NUMERIC_INCRBY = 3
    METHOD_NUMERIC_DECRBY = 4

    METHOD_TTL_PEXPIRE = 1
    METHOD_TTL_PTTL = 2
    METHOD_TTL_PERSIST = 3
    METHOD_TTL_PEXPIREAT = 4

    def set(self, key: str, value: str | bytes):
        req = kv_pb2.SetRequest()
        req.key = key
        req.value = value.encode("utf-8") if isinstance(value, str) else value
        return self._call(self.SERVICE_KV, self.METHOD_SET, req, kv_pb2.SetResponse)

    def get(self, key: str):
        req = kv_pb2.GetRequest(key=key)
        return self._call(self.SERVICE_KV, self.METHOD_GET, req, kv_pb2.GetResponse)

    def delete(self, key: str):
        req = kv_pb2.DelRequest(key=key)
        return self._call(self.SERVICE_KV, self.METHOD_DEL, req, kv_pb2.DelResponse)

    def exists(self, key: str):
        req = kv_pb2.ExistsRequest(key=key)
        return self._call(self.SERVICE_KV, self.METHOD_EXISTS, req, kv_pb2.ExistsResponse)

    def size(self):
        req = kv_pb2.SizeRequest()
        return self._call(self.SERVICE_KV, self.METHOD_SIZE, req, kv_pb2.SizeResponse)

    def incr(self, key: str, kind: int):
        req = numeric_pb2.NumericKeyRequest(key=key, kind=kind)
        return self._call(
            self.SERVICE_NUMERIC,
            self.METHOD_NUMERIC_INCR,
            req,
            numeric_pb2.NumericValueResponse,
        )

    def decr(self, key: str, kind: int):
        req = numeric_pb2.NumericKeyRequest(key=key, kind=kind)
        return self._call(
            self.SERVICE_NUMERIC,
            self.METHOD_NUMERIC_DECR,
            req,
            numeric_pb2.NumericValueResponse,
        )

    def incrby_int(self, key: str, delta: int):
        req = numeric_pb2.NumericDeltaRequest(key=key, int_delta=delta)
        return self._call(
            self.SERVICE_NUMERIC,
            self.METHOD_NUMERIC_INCRBY,
            req,
            numeric_pb2.NumericValueResponse,
        )

    def incrby_float(self, key: str, delta: float):
        req = numeric_pb2.NumericDeltaRequest(key=key, float_delta=delta)
        return self._call(
            self.SERVICE_NUMERIC,
            self.METHOD_NUMERIC_INCRBY,
            req,
            numeric_pb2.NumericValueResponse,
        )

    def decrby_int(self, key: str, delta: int):
        req = numeric_pb2.NumericDeltaRequest(key=key, int_delta=delta)
        return self._call(
            self.SERVICE_NUMERIC,
            self.METHOD_NUMERIC_DECRBY,
            req,
            numeric_pb2.NumericValueResponse,
        )

    def decrby_float(self, key: str, delta: float):
        req = numeric_pb2.NumericDeltaRequest(key=key, float_delta=delta)
        return self._call(
            self.SERVICE_NUMERIC,
            self.METHOD_NUMERIC_DECRBY,
            req,
            numeric_pb2.NumericValueResponse,
        )

    def pexpire(self, key: str, ttl_ms: int):
        req = ttl_pb2.PExpireRequest(key=key, ttl_ms=ttl_ms)
        return self._call(
            self.SERVICE_TTL,
            self.METHOD_TTL_PEXPIRE,
            req,
            ttl_pb2.PExpireResponse,
        )

    def pttl(self, key: str):
        req = ttl_pb2.TtlKeyRequest(key=key)
        return self._call(
            self.SERVICE_TTL,
            self.METHOD_TTL_PTTL,
            req,
            ttl_pb2.PTtlResponse,
        )

    def persist(self, key: str):
        req = ttl_pb2.TtlKeyRequest(key=key)
        return self._call(
            self.SERVICE_TTL,
            self.METHOD_TTL_PERSIST,
            req,
            ttl_pb2.PersistResponse,
        )

    def pexpireat(self, key: str, expire_at_ms: int):
        req = ttl_pb2.PExpireAtRequest(key=key, expire_at_ms=expire_at_ms)
        return self._call(
            self.SERVICE_TTL,
            self.METHOD_TTL_PEXPIREAT,
            req,
            ttl_pb2.PExpireResponse,
        )


def response_has_field(msg, field_name: str) -> bool:
    try:
        return msg.HasField(field_name)
    except ValueError:
        return False


def enum_name(enum_cls, value: int) -> str:
    try:
        return enum_cls.Name(value)
    except Exception:
        return str(value)


def format_get_response(resp) -> str:
    if response_has_field(resp, "value"):
        try:
            return f"value={resp.value.decode('utf-8')!r}"
        except UnicodeDecodeError:
            return f"value=<bytes {resp.value!r}>"

    if getattr(resp, "value", b""):
        try:
            return f"value={resp.value.decode('utf-8')!r}"
        except UnicodeDecodeError:
            return f"value=<bytes {resp.value!r}>"

    return "value=<missing>"


def format_numeric_response(resp) -> str:
    which = resp.WhichOneof("value")
    if which == "int_value":
        return f"int_value={resp.int_value}"
    if which == "float_value":
        return f"float_value={resp.float_value}"
    return "value=<missing>"


def format_pexpire_response(resp) -> str:
    state = enum_name(ttl_pb2.PExpireState, resp.state)
    return f"state={state} expire_at_ms={resp.expire_at_ms}"


def format_pttl_response(resp) -> str:
    state = enum_name(ttl_pb2.PTtlState, resp.state)
    if resp.state == ttl_pb2.PTTL_STATE_HAS_EXPIRE:
        return f"state={state} remaining_ms={resp.remaining_ms}"
    return f"state={state}"


def format_persist_response(resp) -> str:
    fields = resp.DESCRIPTOR.fields_by_name
    if "state" in fields and hasattr(ttl_pb2, "PersistState"):
        state = enum_name(ttl_pb2.PersistState, resp.state)
        return f"state={state}"
    return f"updated={resp.updated}"


def run_kv_smoke(client: KvdClient) -> None:
    print("[1] SIZE")
    r = client.size()
    print(f"size={r.size}")

    print("[2] SET hello world")
    r = client.set("hello", "world")
    print(f"applied={r.applied}")

    print("[3] EXISTS hello")
    r = client.exists("hello")
    print(f"exists={r.exists}")

    print("[4] GET hello")
    r = client.get("hello")
    print(format_get_response(r))

    print("[5] DEL hello")
    r = client.delete("hello")
    print(f"removed={r.removed}")

    print("[6] EXISTS hello")
    r = client.exists("hello")
    print(f"exists={r.exists}")

    print("[7] GET hello")
    r = client.get("hello")
    print(format_get_response(r))

    print("[8] SIZE")
    r = client.size()
    print(f"size={r.size}")


def run_numeric_smoke(client: KvdClient) -> None:
    int_key = "num:int:smoke"
    float_key = "num:float:smoke"

    print("[1] SET int key = 10")
    client.set(int_key, "10")

    print("[2] INCR int")
    r = client.incr(int_key, numeric_pb2.NUMERIC_KIND_INT)
    print(format_numeric_response(r))

    print("[3] INCRBY int +5")
    r = client.incrby_int(int_key, 5)
    print(format_numeric_response(r))

    print("[4] DECRBY int -3")
    r = client.decrby_int(int_key, 3)
    print(format_numeric_response(r))

    print("[5] SET float key = 1.5")
    client.set(float_key, "1.5")

    print("[6] INCR float")
    r = client.incr(float_key, numeric_pb2.NUMERIC_KIND_FLOAT)
    print(format_numeric_response(r))

    print("[7] INCRBY float +0.25")
    r = client.incrby_float(float_key, 0.25)
    print(format_numeric_response(r))

    print("[8] DECRBY float -0.5")
    r = client.decrby_float(float_key, 0.5)
    print(format_numeric_response(r))


def run_ttl_smoke(client: KvdClient) -> None:
    expire_key = "ttl:expire:smoke"
    persist_key = "ttl:persist:smoke"
    abs_key = "ttl:at:smoke"

    print("[1] SET expire key")
    r = client.set(expire_key, "value")
    print(f"applied={r.applied}")

    print("[2] PEXPIRE expire key 300ms")
    r = client.pexpire(expire_key, 300)
    print(format_pexpire_response(r))

    print("[3] PTTL expire key")
    r = client.pttl(expire_key)
    print(format_pttl_response(r))

    print("[4] sleep 0.45s")
    time.sleep(0.45)

    print("[5] EXISTS expire key")
    r = client.exists(expire_key)
    print(f"exists={r.exists}")

    print("[6] SET persist key")
    r = client.set(persist_key, "value")
    print(f"applied={r.applied}")

    print("[7] PEXPIRE persist key 1000ms")
    r = client.pexpire(persist_key, 1000)
    print(format_pexpire_response(r))

    print("[8] PERSIST persist key")
    r = client.persist(persist_key)
    print(format_persist_response(r))

    print("[9] PTTL persist key")
    r = client.pttl(persist_key)
    print(format_pttl_response(r))

    print("[10] EXISTS persist key")
    r = client.exists(persist_key)
    print(f"exists={r.exists}")

    print("[11] SET abs key")
    r = client.set(abs_key, "value")
    print(f"applied={r.applied}")

    expire_at_ms = int(time.time() * 1000) + 400
    print(f"[12] PEXPIREAT abs key {expire_at_ms}")
    r = client.pexpireat(abs_key, expire_at_ms)
    print(format_pexpire_response(r))

    print("[13] PTTL abs key")
    r = client.pttl(abs_key)
    print(format_pttl_response(r))

    print("[14] sleep 0.55s")
    time.sleep(0.55)

    print("[15] EXISTS abs key")
    r = client.exists(abs_key)
    print(f"exists={r.exists}")