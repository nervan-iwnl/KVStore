cmake_minimum_required(VERSION 3.16)
project(kvd LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(kvd
    apps/kvd/main.cpp

    src/app/dispatch.cpp
    src/app/command_registry.cpp
    src/app/handlers/handlers_admin.cpp
    src/app/handlers/handlers_kv.cpp
    src/app/handlers/handlers_numeric.cpp
    src/app/handlers/handlers_ttl.cpp

    src/domain/kv_store.cpp

    src/server/socket_io.cpp
    src/server/server.cpp
    src/server/session.cpp

    src/spec/schemas.cpp

    src/transport/kvn1/codec.cpp
    src/transport/kvn1/adapter.cpp

    src/ttl/cleaner.cpp
    src/ttl/wheel.cpp

    src/util/utils.cpp
)

target_include_directories(kvd PRIVATE include)

find_package(Threads REQUIRED)
target_link_libraries(kvd PRIVATE Threads::Threads)

if (MSVC)
    target_compile_options(kvd PRIVATE /W4)
else()
    target_compile_options(kvd PRIVATE -Wall -Wextra -Wpedantic)
endif()
