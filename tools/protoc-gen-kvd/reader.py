from __future__ import annotations

from pathlib import PurePosixPath

from google.protobuf.compiler import plugin_pb2 as plugin_pb2
from google.protobuf.descriptor_pb2 import (
    FileDescriptorProto,
    MethodDescriptorProto,
    ServiceDescriptorProto,
)

import kvd.options_pb2 as options_pb2
from model import FileModel, MethodModel, ServiceModel


def load_models(req: plugin_pb2.CodeGeneratorRequest) -> list[FileModel]:
    target_files = set(req.file_to_generate)
    result: list[FileModel] = []

    for fd in req.proto_file:
        if fd.name not in target_files:
            continue

        if not fd.service:
            continue

        result.append(_build_file_model(fd))

    return result


def _build_file_model(fd: FileDescriptorProto) -> FileModel:
    file_model = FileModel(
        proto_file=fd.name,
        proto_base=_proto_base(fd.name),
    )

    for svc in fd.service:
        file_model.services.append(_build_service_model(fd, svc))

    return file_model


def _build_service_model(
    fd: FileDescriptorProto,
    svc: ServiceDescriptorProto,
) -> ServiceModel:
    service_model = ServiceModel(
        name=svc.name,
        service_id=_read_service_id(fd, svc),
    )

    for method in svc.method:
        service_model.methods.append(_build_method_model(fd, svc, method))

    return service_model


def _build_method_model(
    fd: FileDescriptorProto,
    svc: ServiceDescriptorProto,
    method: MethodDescriptorProto,
) -> MethodModel:
    return MethodModel(
        name=method.name,
        request_cpp=_proto_type_to_cpp(method.input_type),
        response_cpp=_proto_type_to_cpp(method.output_type),
        method_id=_read_method_id(fd, svc, method),
        client_streaming=method.client_streaming,
        server_streaming=method.server_streaming,
    )


def _read_service_id(
    fd: FileDescriptorProto,
    svc: ServiceDescriptorProto,
) -> int:
    opts = svc.options

    if not opts.HasExtension(options_pb2.service_id):
        raise ValueError(
            f"{fd.name}: service {svc.name} has no (kvd.options.service_id)"
        )

    value = int(opts.Extensions[options_pb2.service_id])
    if value <= 0:
        raise ValueError(
            f"{fd.name}: service {svc.name} has invalid service_id={value}"
        )

    return value


def _read_method_id(
    fd: FileDescriptorProto,
    svc: ServiceDescriptorProto,
    method: MethodDescriptorProto,
) -> int:
    opts = method.options

    if not opts.HasExtension(options_pb2.method_id):
        raise ValueError(
            f"{fd.name}: method {svc.name}.{method.name} has no (kvd.options.method_id)"
        )

    value = int(opts.Extensions[options_pb2.method_id])
    if value <= 0:
        raise ValueError(
            f"{fd.name}: method {svc.name}.{method.name} has invalid method_id={value}"
        )

    return value


def _proto_base(proto_file: str) -> str:
    return str(PurePosixPath(proto_file).with_suffix(""))


def _proto_type_to_cpp(proto_name: str) -> str:
    if not proto_name:
        raise ValueError("empty protobuf type name")

    if proto_name.startswith("."):
        proto_name = proto_name[1:]

    return "::" + proto_name.replace(".", "::")