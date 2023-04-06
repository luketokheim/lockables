cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    lockables
    VERSION 0.1.0
    DESCRIPTION "Lockables"
    HOMEPAGE_URL "https://github.com/luketokheim/lockables"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(lockables_lockables INTERFACE)
add_library(lockables::lockables ALIAS lockables_lockables)

set_property(
    TARGET lockables_lockables PROPERTY
    EXPORT_NAME lockables
)

target_include_directories(
    lockables_lockables ${warning_guard}
    INTERFACE
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_compile_features(lockables_lockables INTERFACE cxx_std_17)

find_package(fmt REQUIRED)
target_link_libraries(lockables_lockables INTERFACE fmt::fmt)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT lockables_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of lockables"
  )
endif()

include(cmake/dev-mode.cmake)
