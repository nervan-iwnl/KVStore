from __future__ import annotations

from contextlib import contextmanager
from dataclasses import dataclass, field

from google.protobuf.compiler import plugin_pb2 as plugin_pb2

from model import FileModel, MethodKind, MethodModel, ServiceModel
from writer import make_file


@dataclass
class TextWriter:
    indent_str: str = "    "
    level: int = 0
    lines: list[str] = field(default_factory=list)

    def line(self, text: str = "") -> None:
        if text:
            self.lines.append(f"{self.indent_str * self.level}{text}")
        else:
            self.lines.append("")

    def blank(self) -> None:
        if not self.lines or self.lines[-1] != "":
            self.lines.append("")

    def indent(self) -> None:
        self.level += 1

    def dedent(self) -> None:
        if self.level == 0:
            raise ValueError("indent level is already zero")
        self.level -= 1

    @contextmanager
    def block(self, header: str, footer: str = "}"):
        self.line(f"{header} {{")
        self.indent()
        try:
            yield
        finally:
            self.dedent()
            self.line(footer)

    def render(self) -> str:
        return "\n".join(self.lines)


def render_files(
    files: list[FileModel],
) -> list[plugin_pb2.CodeGeneratorResponse.File]:
    out: list[plugin_pb2.CodeGeneratorResponse.File] = []

    if files:
        out.append(
            make_file(
                _common_header_name(),
                _render_common_header(),
            )
        )

    for file_model in files:
        _assert_supported_file(file_model)

        out.append(make_file(_header_name(file_model), _render_header(file_model)))
        out.append(make_file(_source_name(file_model), _render_source(file_model)))

    return out


def _render_header(file_model: FileModel) -> str:
    w = TextWriter()

    w.line("#pragma once")
    w.blank()
    w.line(f'#include "{_pb_header_name(file_model)}"')
    w.line('#include "kvd/dispatch_common.gen.hpp"')
    w.line('#include "kvd/api/v1/envelope.pb.h"')
    w.line('#include "runtime/request_context.hpp"')
    w.line('#include "runtime/status.hpp"')
    w.line('#include "transport/dispatcher.hpp"')
    w.line("#include <cstdint>")
    w.blank()

    with w.block("namespace kvd::gen", "}  // namespace kvd::gen"):
        for i, service in enumerate(file_model.services):
            if i:
                w.blank()
            _emit_service_class_decl(w, service)
            w.blank()
            _emit_register_decl(w, service)
            w.blank()
            _emit_find_desc_decl(w, service)

    w.blank()

    with w.block("namespace kvd::gen::ids", "}  // namespace kvd::gen::ids"):
        for i, service in enumerate(file_model.services):
            if i:
                w.blank()
            _emit_service_ids(w, service)

    w.blank()
    return w.render()


def _render_source(file_model: FileModel) -> str:
    w = TextWriter()

    w.line(f'#include "{_header_name(file_model)}"')
    w.line('#include "runtime/dispatch_helpers.hpp"')
    w.blank()

    with w.block("namespace kvd::gen", "}  // namespace kvd::gen"):
        for service in file_model.services:
            for method in service.methods:
                _emit_method_desc_var(w, service, method)

        if file_model.services:
            w.blank()

        for i, service in enumerate(file_model.services):
            _emit_find_desc_impl(w, service)
            w.blank()
            _emit_handler_functions(w, service)
            w.blank()
            _emit_register_impl(w, service)

            if i != len(file_model.services) - 1:
                w.blank()

    w.blank()
    return w.render()


def _common_header_name() -> str:
    return "kvd/dispatch_common.gen.hpp"


def _render_common_header() -> str:
    w = TextWriter()

    w.line("#pragma once")
    w.blank()
    w.line("#include <cstdint>")
    w.blank()

    with w.block("namespace kvd::gen", "}  // namespace kvd::gen"):
        with w.block("enum class MethodKind : std::uint8_t", "};"):
            w.line("kUnary = 0,")
            w.line("kClientStream = 1,")
            w.line("kServerStream = 2,")
            w.line("kBidiStream = 3,")

        w.blank()

        with w.block("struct MethodDesc", "};"):
            w.line("std::uint32_t service_id;")
            w.line("std::uint32_t method_id;")
            w.line("const char* service_name;")
            w.line("const char* method_name;")
            w.line("MethodKind kind;")

    w.blank()
    return w.render()


def _emit_service_class_decl(w: TextWriter, service: ServiceModel) -> None:
    with w.block(f"class {service.name}Service", "};"):
        w.line("public:")
        w.indent()
        w.line(f"virtual ~{service.name}Service() = default;")
        w.blank()

        for method in service.methods:
            _assert_unary_method(service, method)
            w.line("virtual kvd::runtime::Status")
            w.line(f"{method.name}(")
            w.indent()
            w.line(f"const {method.request_cpp}& req,")
            w.line(f"{method.response_cpp}& resp,")
            w.line("kvd::runtime::RequestContext& ctx")
            w.dedent()
            w.line(") = 0;")
            w.blank()

        # убираем лишнюю пустую строку перед закрытием класса
        if w.lines and w.lines[-1] == "":
            w.lines.pop()

        w.dedent()


def _emit_register_decl(w: TextWriter, service: ServiceModel) -> None:
    w.line(f"void Register{service.name}Service(")
    w.indent()
    w.line("kvd::transport::Dispatcher& dispatcher,")
    w.line(f"{service.name}Service& service")
    w.dedent()
    w.line(");")


def _emit_find_desc_decl(w: TextWriter, service: ServiceModel) -> None:
    w.line(
        f"const MethodDesc* Find{service.name}MethodDesc(std::uint32_t method_id);"
    )


def _emit_service_ids(w: TextWriter, service: ServiceModel) -> None:
    with w.block(f"namespace {service.name}", f"}}  // namespace {service.name}"):
        w.line(f"inline constexpr std::uint32_t kServiceId = {service.service_id};")
        for method in service.methods:
            w.line(
                f"inline constexpr std::uint32_t k{method.name}MethodId = {method.method_id};"
            )


def _emit_method_desc_var(
    w: TextWriter,
    service: ServiceModel,
    method: MethodModel,
) -> None:
    with w.block(
        f"static constexpr MethodDesc {_method_desc_var_name(service, method)}",
        "};",
    ):
        w.line(f"{service.service_id},")
        w.line(f"{method.method_id},")
        w.line(f'"{service.name}",')
        w.line(f'"{method.name}",')
        w.line(f"{_method_kind_cpp(method.kind)},")


def _emit_find_desc_impl(w: TextWriter, service: ServiceModel) -> None:
    with w.block(
        f"const MethodDesc* Find{service.name}MethodDesc(std::uint32_t method_id)"
    ):
        with w.block("switch (method_id)"):
            for method in service.methods:
                w.line(f"case {method.method_id}:")
                w.indent()
                w.line(f"return &{_method_desc_var_name(service, method)};")
                w.dedent()

            w.line("default:")
            w.indent()
            w.line("return nullptr;")
            w.dedent()


def _emit_handler_functions(w: TextWriter, service: ServiceModel) -> None:
    for method in service.methods:
        _emit_unary_handler_function(w, service, method)
        w.blank()

    if w.lines and w.lines[-1] == "":
        w.lines.pop()


def _emit_unary_handler_function(
    w: TextWriter,
    service: ServiceModel,
    method: MethodModel,
) -> None:
    req_cpp = method.request_cpp
    resp_cpp = method.response_cpp

    signature = (
        f"static void Handle_{service.name}_{method.name}("
        f"{service.name}Service& service, "
        f"const ::kvd::api::v1::RequestFrame& req_frame, "
        f"::kvd::api::v1::ResponseFrame& resp_frame, "
        f"kvd::runtime::RequestContext& ctx)"
    )

    with w.block(signature):
        w.line(f"{req_cpp} req;")
        w.line(f"{resp_cpp} resp;")
        w.blank()

        w.line("resp_frame.set_protocol_version(req_frame.protocol_version());")
        w.line("resp_frame.set_request_id(req_frame.request_id());")
        w.blank()

        with w.block('if (!req.ParseFromString(req_frame.payload()))'):
            w.line(
                'kvd::runtime::fill_error('
                'resp_frame, '
                '::kvd::api::v1::ProtoError::BAD_REQUEST, '
                '"bad protobuf payload");'
            )
            w.line("return;")

        w.blank()
        w.line(
            f"kvd::runtime::Status st = service.{method.name}(req, resp, ctx);"
        )
        w.blank()

        with w.block("if (!st.ok)"):
            w.line("kvd::runtime::fill_error(resp_frame, st.code, st.message);")
            w.line("return;")

        w.blank()
        with w.block("if (!kvd::runtime::fill_serialized_payload(resp_frame, resp))"):
            w.line(
                'kvd::runtime::fill_error('
                'resp_frame, '
                '::kvd::api::v1::ProtoError::INTERNAL, '
                '"failed to serialize protobuf response");'
            )
            w.line("return;")


def _emit_register_impl(w: TextWriter, service: ServiceModel) -> None:
    signature = (
        f"void Register{service.name}Service("
        f"kvd::transport::Dispatcher& dispatcher, "
        f"{service.name}Service& service)"
    )

    with w.block(signature):
        for method in service.methods:
            with w.block(
                f"dispatcher.Register({service.service_id}, {method.method_id}, "
                f"[&service](const ::kvd::api::v1::RequestFrame& req_frame, "
                f"::kvd::api::v1::ResponseFrame& resp_frame, "
                f"kvd::runtime::RequestContext& ctx)",
                "});",
            ):
                w.line(
                    f"Handle_{service.name}_{method.name}(service, req_frame, resp_frame, ctx);"
                )


def _header_name(file_model: FileModel) -> str:
    return f"{file_model.proto_base}.dispatch.gen.hpp"


def _source_name(file_model: FileModel) -> str:
    return f"{file_model.proto_base}.dispatch.gen.cpp"


def _pb_header_name(file_model: FileModel) -> str:
    return f"{file_model.proto_base}.pb.h"


def _method_desc_var_name(service: ServiceModel, method: MethodModel) -> str:
    return f"kMethodDesc_{service.name}_{method.name}"


def _method_kind_cpp(kind: MethodKind) -> str:
    if kind == MethodKind.UNARY:
        return "MethodKind::kUnary"
    if kind == MethodKind.CLIENT_STREAM:
        return "MethodKind::kClientStream"
    if kind == MethodKind.SERVER_STREAM:
        return "MethodKind::kServerStream"
    if kind == MethodKind.BIDI_STREAM:
        return "MethodKind::kBidiStream"
    raise ValueError(f"unknown method kind: {kind}")


def _assert_supported_file(file_model: FileModel) -> None:
    if not file_model.services:
        raise ValueError(f"{file_model.proto_file}: no services to render")

    for service in file_model.services:
        for method in service.methods:
            _assert_unary_method(service, method)


def _assert_unary_method(service: ServiceModel, method: MethodModel) -> None:
    if method.kind != MethodKind.UNARY:
        raise ValueError(
            f"renderer: method {service.name}.{method.name} has kind={method.kind.value}, "
            "but only unary rendering is implemented now"
        )