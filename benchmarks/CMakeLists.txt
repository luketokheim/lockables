cmake_minimum_required(VERSION 3.14)

project(lockablesBenchmarks LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
    find_package(lockables REQUIRED)
endif()

find_package(benchmark REQUIRED)

# ---- Benchmarks ----

add_executable(lockables-bench bench.cpp bench_guarded.cpp bench_value.cpp)
target_link_libraries(
    lockables-bench PRIVATE
    lockables::lockables
    benchmark::benchmark
)

# ---- End-of-file commands ----

add_folders(Benchmarks)
