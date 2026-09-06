cmake_minimum_required(VERSION 3.20)
project(CortexKernelStorage)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_library(cortexkernel_core
    src/graph_store.cpp
)
target_include_directories(cortexkernel_core PUBLIC include)

add_executable(storage_server src/main.cpp)
target_link_libraries(storage_server PRIVATE cortexkernel_core Threads::Threads)