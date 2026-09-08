cmake_minimum_required(VERSION 3.20)
project(CortexKernelStorage)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
)
FetchContent_MakeAvailable(Catch2)

add_library(cortexkernel_core
    src/graph_store.cpp
    src/json_translation.cpp
    src/contradiction_detector.cpp
    src/persistence.cpp
    src/semantic_index.cpp
    src/logger.cpp
    src/metrics.cpp
    src/redis_client.cpp
    src/node_cache.cpp
)
target_include_directories(cortexkernel_core PUBLIC include)


add_executable(storage_server src/main.cpp)
target_link_libraries(storage_server PRIVATE cortexkernel_core Threads::Threads)

add_executable(contradiction_bench bench/contradiction_bench.cpp)
target_link_libraries(contradiction_bench PRIVATE cortexkernel_core)

add_executable(semantic_index_bench bench/semantic_index_bench.cpp)
target_link_libraries(semantic_index_bench PRIVATE cortexkernel_core)

find_library(SQLITE3_LIBRARY sqlite3 REQUIRED)
add_executable(sqlite_baseline_bench bench/sqlite_baseline_bench.cpp)
target_link_libraries(sqlite_baseline_bench PRIVATE ${SQLITE3_LIBRARY})

find_package(Protobuf CONFIG QUIET)
find_package(gRPC CONFIG QUIET)
set(CORTEXKERNEL_GRPC_VIA_PKGCONFIG FALSE)

if(NOT (Protobuf_FOUND AND gRPC_FOUND))
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(GRPCXX QUIET IMPORTED_TARGET grpc++)
        pkg_check_modules(PROTOBUF_PKG QUIET IMPORTED_TARGET protobuf)
        if(GRPCXX_FOUND AND PROTOBUF_PKG_FOUND)
            set(Protobuf_FOUND TRUE)
            set(gRPC_FOUND TRUE)
            set(CORTEXKERNEL_GRPC_VIA_PKGCONFIG TRUE)
            message(STATUS "protobuf and gRPC found via pkg-config (no CMake Config package on this system)")
        endif()
    endif()
endif()

if(Protobuf_FOUND AND gRPC_FOUND)
    message(STATUS "protobuf and gRPC found - building the gRPC storage service")

    set(PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../proto)
    set(PROTO_FILE ${PROTO_DIR}/cortexkernel.proto)
    set(GENERATED_PROTO_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
    file(MAKE_DIRECTORY ${GENERATED_PROTO_DIR})

    find_program(PROTOC_EXECUTABLE protoc REQUIRED)
    find_program(GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin REQUIRED)

    set(PROTO_SRCS ${GENERATED_PROTO_DIR}/cortexkernel.pb.cc)
    set(PROTO_HDRS ${GENERATED_PROTO_DIR}/cortexkernel.pb.h)
    set(GRPC_SRCS ${GENERATED_PROTO_DIR}/cortexkernel.grpc.pb.cc)
    set(GRPC_HDRS ${GENERATED_PROTO_DIR}/cortexkernel.grpc.pb.h)

    add_custom_command(
        OUTPUT ${PROTO_SRCS} ${PROTO_HDRS} ${GRPC_SRCS} ${GRPC_HDRS}
        COMMAND ${PROTOC_EXECUTABLE}
        ARGS --cpp_out=${GENERATED_PROTO_DIR}
             --grpc_out=${GENERATED_PROTO_DIR}
             --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN_EXECUTABLE}
             -I ${PROTO_DIR}
             ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
        COMMENT "Generating C++ code from cortexkernel.proto"
    )

    add_library(cortexkernel_proto ${PROTO_SRCS} ${GRPC_SRCS})
    if(CORTEXKERNEL_GRPC_VIA_PKGCONFIG)
        target_link_libraries(cortexkernel_proto PUBLIC PkgConfig::GRPCXX PkgConfig::PROTOBUF_PKG)
    else()
        target_link_libraries(cortexkernel_proto PUBLIC protobuf::libprotobuf gRPC::grpc++)
    endif()
    target_include_directories(cortexkernel_proto PUBLIC ${GENERATED_PROTO_DIR})

    add_library(cortexkernel_grpc_service src/grpc_server.cpp)
    target_link_libraries(cortexkernel_grpc_service PUBLIC cortexkernel_core cortexkernel_proto)

    target_link_libraries(storage_server PRIVATE cortexkernel_grpc_service)
    target_compile_definitions(storage_server PRIVATE CORTEXKERNEL_GRPC_ENABLED)
else()
    message(STATUS "protobuf/gRPC not found - skipping the gRPC storage service (REST API unaffected)")
endif()

add_executable(unit_tests
    tests/test_graph_store.cpp
    tests/test_json_translation.cpp
    tests/test_contradiction_detector.cpp
    tests/test_semantic_index.cpp
    tests/test_metrics.cpp
    tests/test_redis_client.cpp
)
target_link_libraries(unit_tests PRIVATE cortexkernel_core Catch2::Catch2WithMain)

if(Protobuf_FOUND AND gRPC_FOUND)
    target_sources(unit_tests PRIVATE tests/test_grpc_server.cpp)
    target_link_libraries(unit_tests PRIVATE cortexkernel_grpc_service)
endif()

include(CTest)
include(Catch)
catch_discover_tests(unit_tests)