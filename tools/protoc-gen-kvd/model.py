from __future__ import annotations

from dataclasses import dataclass, field
from enum import Enum


class MethodKind(str, Enum):
    UNARY = "unary"
    CLIENT_STREAM = "client_stream"
    SERVER_STREAM = "server_stream"
    BIDI_STREAM = "bidi_stream"


@dataclass
class MethodModel:
    name: str
    request_cpp: str
    response_cpp: str
    method_id: int
    client_streaming: bool = False
    server_streaming: bool = False
    kind: MethodKind = MethodKind.UNARY


@dataclass
class ServiceModel:
    name: str
    service_id: int
    methods: list[MethodModel] = field(default_factory=list)


@dataclass
class FileModel:
    proto_file: str
    proto_base: str
    services: list[ServiceModel] = field(default_factory=list)