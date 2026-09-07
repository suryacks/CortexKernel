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
)
target_include_directories(cortexkernel_core PUBLIC include)


add_executable(storage_server src/main.cpp)
target_link_libraries(storage_server PRIVATE cortexkernel_core Threads::Threads)

add_executable(contradiction_bench bench/contradiction_bench.cpp)
target_link_libraries(contradiction_bench PRIVATE cortexkernel_core)

add_executable(semantic_index_bench bench/semantic_index_bench.cpp)
target_link_libraries(semantic_index_bench PRIVATE cortexkernel_core)

add_executable(unit_tests
    tests/test_graph_store.cpp
    tests/test_json_translation.cpp
    tests/test_contradiction_detector.cpp
    tests/test_semantic_index.cpp
)
target_link_libraries(unit_tests PRIVATE cortexkernel_core Catch2::Catch2WithMain)
include(CTest)
include(Catch)
catch_discover_tests(unit_tests)