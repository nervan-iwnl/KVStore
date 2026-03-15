from __future__ import annotations

import argparse
import sys

from .client import (
    KvdClient,
    format_get_response,
    format_numeric_response,
    format_pexpire_response,
    format_persist_response,
    format_pttl_response,
    run_kv_smoke,
    run_numeric_smoke,
    run_ttl_smoke,
)
from .proto_runtime import numeric_pb2
from .transport import RpcError


def parse_numeric_kind(value: str) -> int:
    value = value.strip().lower()
    if value == "int":
        return numeric_pb2.NUMERIC_KIND_INT
    if value == "float":
        return numeric_pb2.NUMERIC_KIND_FLOAT
    raise argparse.ArgumentTypeError("kind must be 'int' or 'float'")


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description="KVD protobuf test client")
    p.add_argument("--host", default="127.0.0.1")
    p.add_argument("--port", type=int, default=7777)
    p.add_argument("--timeout", type=float, default=5.0)

    sub = p.add_subparsers(dest="cmd", required=True)

    sub.add_parser("smoke")
    sub.add_parser("kv-smoke")
    sub.add_parser("numeric-smoke")
    sub.add_parser("ttl-smoke")
    sub.add_parser("size")

    sp = sub.add_parser("set")
    sp.add_argument("key")
    sp.add_argument("value")

    sp = sub.add_parser("get")
    sp.add_argument("key")

    sp = sub.add_parser("del")
    sp.add_argument("key")

    sp = sub.add_parser("exists")
    sp.add_argument("key")

    sp = sub.add_parser("pexpire")
    sp.add_argument("key")
    sp.add_argument("ttl_ms", type=int)

    sp = sub.add_parser("pexpireat")
    sp.add_argument("key")
    sp.add_argument("expire_at_ms", type=int)

    sp = sub.add_parser("pttl")
    sp.add_argument("key")

    sp = sub.add_parser("persist")
    sp.add_argument("key")

    sp = sub.add_parser("incr")
    sp.add_argument("key")
    sp.add_argument("--kind", required=True, type=parse_numeric_kind)

    sp = sub.add_parser("decr")
    sp.add_argument("key")
    sp.add_argument("--kind", required=True, type=parse_numeric_kind)

    sp = sub.add_parser("incrby")
    sp.add_argument("key")
    grp = sp.add_mutually_exclusive_group(required=True)
    grp.add_argument("--int", dest="int_delta", type=int)
    grp.add_argument("--float", dest="float_delta", type=float)

    sp = sub.add_parser("decrby")
    sp.add_argument("key")
    grp = sp.add_mutually_exclusive_group(required=True)
    grp.add_argument("--int", dest="int_delta", type=int)
    grp.add_argument("--float", dest="float_delta", type=float)

    return p


def main() -> int:
    args = build_parser().parse_args()

    try:
        with KvdClient(args.host, args.port, args.timeout) as client:
            if args.cmd == "smoke":
                run_kv_smoke(client)
                print()
                run_numeric_smoke(client)
                print()
                run_ttl_smoke(client)
                return 0

            if args.cmd == "kv-smoke":
                run_kv_smoke(client)
                return 0

            if args.cmd == "numeric-smoke":
                run_numeric_smoke(client)
                return 0

            if args.cmd == "ttl-smoke":
                run_ttl_smoke(client)
                return 0

            if args.cmd == "size":
                r = client.size()
                print(f"size={r.size}")
                return 0

            if args.cmd == "set":
                r = client.set(args.key, args.value)
                print(f"applied={r.applied}")
                return 0

            if args.cmd == "get":
                r = client.get(args.key)
                print(format_get_response(r))
                return 0

            if args.cmd == "del":
                r = client.delete(args.key)
                print(f"removed={r.removed}")
                return 0

            if args.cmd == "exists":
                r = client.exists(args.key)
                print(f"exists={r.exists}")
                return 0

            if args.cmd == "pexpire":
                r = client.pexpire(args.key, args.ttl_ms)
                print(format_pexpire_response(r))
                return 0

            if args.cmd == "pexpireat":
                r = client.pexpireat(args.key, args.expire_at_ms)
                print(format_pexpire_response(r))
                return 0

            if args.cmd == "pttl":
                r = client.pttl(args.key)
                print(format_pttl_response(r))
                return 0

            if args.cmd == "persist":
                r = client.persist(args.key)
                print(format_persist_response(r))
                return 0

            if args.cmd == "incr":
                r = client.incr(args.key, args.kind)
                print(format_numeric_response(r))
                return 0

            if args.cmd == "decr":
                r = client.decr(args.key, args.kind)
                print(format_numeric_response(r))
                return 0

            if args.cmd == "incrby":
                if args.int_delta is not None:
                    r = client.incrby_int(args.key, args.int_delta)
                else:
                    r = client.incrby_float(args.key, args.float_delta)
                print(format_numeric_response(r))
                return 0

            if args.cmd == "decrby":
                if args.int_delta is not None:
                    r = client.decrby_int(args.key, args.int_delta)
                else:
                    r = client.decrby_float(args.key, args.float_delta)
                print(format_numeric_response(r))
                return 0

            raise RuntimeError(f"unknown command: {args.cmd}")

    except RpcError as e:
        print(str(e), file=sys.stderr)
        return 2
    except Exception as e:
        print(f"fatal: {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())