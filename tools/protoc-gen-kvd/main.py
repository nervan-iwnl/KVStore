from __future__ import annotations

import sys

from google.protobuf.compiler import plugin_pb2 as plugin_pb2

from reader import load_models
from renderer import render_files
from validator import validate_files
from writer import make_error_response, make_response, write_response


def read_request_from_stdin() -> plugin_pb2.CodeGeneratorRequest:
    raw = sys.stdin.buffer.read()

    req = plugin_pb2.CodeGeneratorRequest()
    req.ParseFromString(raw)

    return req


def main() -> int:
    try:
        req = read_request_from_stdin()
        files = load_models(req)
        validate_files(files)
        generated_files = render_files(files)

        resp = make_response(generated_files)

    except Exception as e:
        resp = make_error_response(str(e))

    write_response(resp)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())