if(PROJECT_IS_TOP_LEVEL)
  set(
      CMAKE_INSTALL_INCLUDEDIR "include/lockables-${PROJECT_VERSION}"
      CACHE PATH ""
  )
endif()

# Project is configured with no languages, so tell GNUInstallDirs the lib dir
set(CMAKE_INSTALL_LIBDIR lib CACHE PATH "")

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# find_package(<package>) call for consumers to find this project
set(package lockables)

install(
    DIRECTORY include/
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT lockables_Development
)

install(
    TARGETS lockables_lockables
    EXPORT lockablesTargets
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

write_basic_package_version_file(
    "${package}ConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT
)

# Allow package maintainers to freely override the path for the configs
set(
    lockables_INSTALL_CMAKEDIR "${CMAKE_INSTALL_DATADIR}/${package}"
    CACHE PATH "CMake package config location relative to the install prefix"
)
mark_as_advanced(lockables_INSTALL_CMAKEDIR)

install(
    FILES cmake/install-config.cmake
    DESTINATION "${lockables_INSTALL_CMAKEDIR}"
    RENAME "${package}Config.cmake"
    COMPONENT lockables_Development
)

install(
    FILES "${PROJECT_BINARY_DIR}/${package}ConfigVersion.cmake"
    DESTINATION "${lockables_INSTALL_CMAKEDIR}"
    COMPONENT lockables_Development
)

install(
    EXPORT lockablesTargets
    NAMESPACE lockables::
    DESTINATION "${lockables_INSTALL_CMAKEDIR}"
    COMPONENT lockables_Development
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
