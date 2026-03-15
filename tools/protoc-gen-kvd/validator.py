from __future__ import annotations

from model import FileModel, MethodKind

ALLOW_STREAMING = False

def validate_files(files: list[FileModel]) -> None:
    seen_service_ids: dict[int, str] = {}
    
    for file_model in files:
        validate_file(file_model, seen_service_ids)
        
        
def validate_file(
    file_model: FileModel, 
    seen_service_ids: dict[int, str]
) -> None:
    if not file_model.services:
        raise ValueError(
            f'{file_model.proto_file}: no services found to generate'
        )
    
    for service in file_model.services:
        validate_service(file_model, service, seen_service_ids)


def validate_service(
    file_model: FileModel,
    service: ServiceModel,
    seen_service_ids: dict[int, str]
) -> None:
    if not service.name:
        raise ValueError(
            f"{file_model.proto_file}: service with empty name"
        )
        
    if service.service_id <= 0:
        raise ValueError(
            f"{file_model.proto_file}: service {service.name} "
            f"has invalid service_id={service.service_id}"
        )
        
    prev = seen_service_ids.get(service.service_id)
    if prev is not None:
        raise ValueError(
            f"{file_model.proto_file}: duplicate service_id={service.service_id} "
            f"for {service.name}; already used by {prev}"
        )
    cur = f"{file_model.proto_file}:{service.name}"
    seen_service_ids[service.service_id] = cur

    if not service.methods:
        raise ValueError(
            f"{file_model.proto_file}: service {service.name} has no methods"
        )
        
    seen_method_ids: dict[int, str] = {}
    
    for method in service.methods:
        validate_method(file_model, service, method, seen_method_ids)


def validate_method(
    file_model: FileModel,
    service: ServiceModel,
    method: MethodModel,
    seen_method_ids: dict[int, str]
) -> None:
    if not method.name:
        raise ValueError(
            f"{file_model.proto_file}: service {service.name} has method with empty name"
        )

    if method.method_id <= 0:
        raise ValueError(
            f"{file_model.proto_file}: method {service.name}.{method.name} "
            f"has invalid method_id={method.method_id}"
        )

    prev = seen_method_ids.get(method.method_id)
    if prev is not None:
        raise ValueError(
            f"{file_model.proto_file}: duplicate method_id={method.method_id} "
            f"in service {service.name}; methods: {prev} and {method.name}"
        )
    seen_method_ids[method.method_id] = method.name

    if not method.request_cpp:
        raise ValueError(
            f"{file_model.proto_file}: method {service.name}.{method.name} "
            f"has empty request_cpp"
        )

    if not method.response_cpp:
        raise ValueError(
            f"{file_model.proto_file}: method {service.name}.{method.name} "
            f"has empty response_cpp"
        )

    if not ALLOW_STREAMING and method.kind != MethodKind.UNARY:
        raise ValueError(
            f"{file_model.proto_file}: method {service.name}.{method.name} "
            f"uses {method.kind.value}, but runtime supports only unary for now"
        )        