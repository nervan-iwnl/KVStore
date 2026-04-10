cmake_minimum_required(VERSION 3.24)
project(kvd LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

if(NOT COMMAND protobuf_generate)
    message(FATAL_ERROR "protobuf_generate() is not available")
endif()

find_program(
    HOST_PROTOC
    NAMES protoc
    PATHS /usr/bin /usr/local/bin
    REQUIRED
)

message(STATUS "Using protoc: ${HOST_PROTOC}")
message(STATUS "Protobuf version: ${Protobuf_VERSION}")

set(PROTO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/proto")
set(PROTO_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

set(KVD_PLUGIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tools/protoc-gen-kvd")
set(KVD_PLUGIN_BIN "${KVD_PLUGIN_DIR}/protoc-gen-kvd")
set(KVD_PLUGIN_PKG_DIR "${KVD_PLUGIN_DIR}/kvd")
set(KVD_INIT_PY "${KVD_PLUGIN_PKG_DIR}/__init__.py")

set(KVD_REQUIREMENTS_TXT "${KVD_PLUGIN_DIR}/requirements.txt")
set(KVD_VENV_DIR "${KVD_PLUGIN_DIR}/.venv")
set(KVD_VENV_PY "${KVD_VENV_DIR}/bin/python3")
set(KVD_VENV_STAMP "${KVD_VENV_DIR}/.ready")

set(KVD_OPTIONS_PROTO "${PROTO_ROOT}/kvd/options.proto")
set(KVD_OPTIONS_PROTO_REL "kvd/options.proto")
set(KVD_OPTIONS_PY "${KVD_PLUGIN_PKG_DIR}/options_pb2.py")

file(MAKE_DIRECTORY "${PROTO_GEN_DIR}")

set(PROTO_FILES
    "${PROTO_ROOT}/kvd/options.proto"
    "${PROTO_ROOT}/kvd/api/v1/envelope.proto"
    "${PROTO_ROOT}/kvd/api/v1/kv.proto"
    "${PROTO_ROOT}/kvd/api/v1/numeric.proto"
    "${PROTO_ROOT}/kvd/api/v1/ttl.proto"
)

set(SERVICE_PROTO_FILES
    "${PROTO_ROOT}/kvd/api/v1/kv.proto"
    "${PROTO_ROOT}/kvd/api/v1/numeric.proto"
    "${PROTO_ROOT}/kvd/api/v1/ttl.proto"
)

foreach(_proto IN LISTS PROTO_FILES)
    if(NOT EXISTS "${_proto}")
        message(FATAL_ERROR "Missing proto file: ${_proto}")
    endif()
endforeach()

if(NOT EXISTS "${PROTO_ROOT}")
    message(FATAL_ERROR "PROTO_ROOT does not exist: ${PROTO_ROOT}")
endif()

if(NOT EXISTS "${KVD_PLUGIN_BIN}")
    message(FATAL_ERROR "Missing plugin launcher: ${KVD_PLUGIN_BIN}")
endif()

if(NOT EXISTS "${KVD_REQUIREMENTS_TXT}")
    message(FATAL_ERROR "Missing requirements.txt: ${KVD_REQUIREMENTS_TXT}")
endif()

# -----------------------------
# protoc-gen-kvd bootstrap
# -----------------------------

add_custom_command(
    OUTPUT "${KVD_VENV_STAMP}"
    COMMAND "${Python3_EXECUTABLE}" -m venv "${KVD_VENV_DIR}"
    COMMAND "${KVD_VENV_PY}" -m pip install --upgrade pip
    COMMAND "${KVD_VENV_PY}" -m pip install -r "${KVD_REQUIREMENTS_TXT}"
    COMMAND "${CMAKE_COMMAND}" -E touch "${KVD_VENV_STAMP}"
    DEPENDS "${KVD_REQUIREMENTS_TXT}"
    COMMENT "Bootstrapping protoc-gen-kvd virtualenv"
    VERBATIM
)

add_custom_target(kvd_plugin_env
    DEPENDS "${KVD_VENV_STAMP}"
)

add_custom_command(
    OUTPUT "${KVD_INIT_PY}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${KVD_PLUGIN_PKG_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E touch "${KVD_INIT_PY}"
    COMMENT "Ensuring protoc-gen-kvd Python package exists"
    VERBATIM
)

add_custom_command(
    OUTPUT "${KVD_OPTIONS_PY}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${KVD_PLUGIN_PKG_DIR}"
    COMMAND "${HOST_PROTOC}"
            --proto_path=.
            --python_out="${KVD_PLUGIN_DIR}"
            "${KVD_OPTIONS_PROTO_REL}"
    WORKING_DIRECTORY "${PROTO_ROOT}"
    DEPENDS
        kvd_plugin_env
        "${KVD_INIT_PY}"
        "${KVD_OPTIONS_PROTO}"
    COMMENT "Generating Python protobuf for kvd custom options"
    VERBATIM
)

add_custom_target(kvd_plugin_bootstrap
    DEPENDS
        kvd_plugin_env
        "${KVD_INIT_PY}"
        "${KVD_OPTIONS_PY}"
)

set(KVD_PLUGIN_SOURCES
    "${KVD_PLUGIN_BIN}"
    "${KVD_PLUGIN_DIR}/main.py"
    "${KVD_PLUGIN_DIR}/model.py"
    "${KVD_PLUGIN_DIR}/reader.py"
    "${KVD_PLUGIN_DIR}/validator.py"
    "${KVD_PLUGIN_DIR}/renderer.py"
    "${KVD_PLUGIN_DIR}/writer.py"
    "${KVD_INIT_PY}"
    "${KVD_REQUIREMENTS_TXT}"
)

# -----------------------------
# app core
# -----------------------------

add_library(app_core STATIC
    src/config/builder.cpp
    src/config/config.cpp
    src/domain/kv_store.cpp
    src/ttl/cleaner.cpp
    src/ttl/wheel.cpp
    src/util/time.cpp
    src/util/utils.cpp
)

target_include_directories(app_core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(app_core
    PUBLIC
        Threads::Threads
)

# -----------------------------
# protobuf + generated dispatch
# -----------------------------

add_library(proto_core STATIC
    src/runtime/dispatch_helpers.cpp
    src/services/kv_service_impl.cpp
    src/services/numeric_service_impl.cpp
    src/services/ttl_service_impl.cpp
)

target_include_directories(proto_core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${PROTO_GEN_DIR}"
)

target_link_libraries(proto_core
    PUBLIC
        protobuf::libprotobuf
        Threads::Threads
        app_core
)

# ordinary protobuf C++ generation
protobuf_generate(
    TARGET proto_core
    LANGUAGE cpp
    IMPORT_DIRS "${PROTO_ROOT}"
    PROTOC_OUT_DIR "${PROTO_GEN_DIR}"
    PROTOC_EXE "${HOST_PROTOC}"
    PROTOS ${PROTO_FILES}
)

# custom kvd dispatch codegen
protobuf_generate(
    TARGET proto_core
    LANGUAGE kvd
    PLUGIN "protoc-gen-kvd=${KVD_PLUGIN_BIN}"
    IMPORT_DIRS "${PROTO_ROOT}"
    PROTOC_OUT_DIR "${PROTO_GEN_DIR}"
    GENERATE_EXTENSIONS .dispatch.gen.hpp .dispatch.gen.cpp
    PROTOC_EXE "${HOST_PROTOC}"
    PROTOS ${SERVICE_PROTO_FILES}
    DEPENDENCIES
        kvd_plugin_bootstrap
        ${KVD_PLUGIN_SOURCES}
)

# -----------------------------
# server core
# -----------------------------

add_library(server_core STATIC
    src/server/server.cpp
    src/server/session.cpp
    src/server/socket_io.cpp
)

target_include_directories(server_core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${PROTO_GEN_DIR}"
)

target_link_libraries(server_core
    PUBLIC
        Threads::Threads
        protobuf::libprotobuf
        app_core
        proto_core
)

# -----------------------------
# executable
# -----------------------------

add_executable(kvd
    apps/kvd/main.cpp
)

target_include_directories(kvd
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${PROTO_GEN_DIR}"
)

target_link_libraries(kvd
    PRIVATE
        server_core
        proto_core
        app_core
        Threads::Threads
        protobuf::libprotobuf
)

# -----------------------------
# warnings
# -----------------------------

if(MSVC)
    target_compile_options(app_core PRIVATE /W4)
    target_compile_options(proto_core PRIVATE /W4)
    target_compile_options(server_core PRIVATE /W4)
    target_compile_options(kvd PRIVATE /W4)
else()
    target_compile_options(app_core PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(proto_core PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(server_core PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(kvd PRIVATE -Wall -Wextra -Wpedantic)
endif()