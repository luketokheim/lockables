cmake_minimum_required(VERSION 3.14)

project(lockablesTests LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
    find_package(lockables REQUIRED)
    enable_testing()
endif()

find_package(Catch2 REQUIRED)

include(Catch)

# ---- Tests ----

add_executable(lockables-test test_guarded.cpp test.cpp)
target_link_libraries(
    lockables-test PRIVATE
    lockables::lockables
    Catch2::Catch2WithMain
)

catch_discover_tests(lockables-test)

# ---- End-of-file commands ----

add_folders(Tests)
