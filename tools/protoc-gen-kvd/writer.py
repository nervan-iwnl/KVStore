from __future__ import annotations

import sys

from google.protobuf.compiler import plugin_pb2 as plugin_pb2


# plugin.proto: FEATURE_PROTO3_OPTIONAL = 1
PROTOC_SUPPORTED_FEATURES = 1


def make_file(name: str, content: str) -> plugin_pb2.CodeGeneratorResponse.File:
    file = plugin_pb2.CodeGeneratorResponse.File()
    file.name = name
    file.content = content
    return file


def make_response(
    files: list[plugin_pb2.CodeGeneratorResponse.File],
) -> plugin_pb2.CodeGeneratorResponse:
    resp = plugin_pb2.CodeGeneratorResponse()
    resp.supported_features = PROTOC_SUPPORTED_FEATURES
    resp.file.extend(files)
    return resp


def make_error_response(message: str) -> plugin_pb2.CodeGeneratorResponse:
    resp = plugin_pb2.CodeGeneratorResponse()
    resp.supported_features = PROTOC_SUPPORTED_FEATURES
    resp.error = message
    return resp


def write_response(resp: plugin_pb2.CodeGeneratorResponse) -> None:
    sys.stdout.buffer.write(resp.SerializeToString())