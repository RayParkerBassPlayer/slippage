cmake_minimum_required(VERSION 3.10)
project(Slippage VERSION 2025.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Configure version header
configure_file(
    ${CMAKE_SOURCE_DIR}/version.hpp.in
    ${CMAKE_BINARY_DIR}/version.hpp
)

include_directories(${CMAKE_SOURCE_DIR}/external/csv-parser/single_include)
include_directories(${CMAKE_BINARY_DIR})

# Library target
add_library(slippage_lib STATIC
    dimensions.cpp
    slip.cpp
    member.cpp
    assignment.cpp
    csv_parser.cpp
    assignment_engine.cpp
)

target_include_directories(slippage_lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/external/csv-parser/single_include>
    $<INSTALL_INTERFACE:include>
    $<INSTALL_INTERFACE:include/slippage/external>
)

set_target_properties(slippage_lib PROPERTIES
    OUTPUT_NAME slippage
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "dimensions.hpp;slip.hpp;member.hpp;assignment.hpp;csv_parser.hpp;assignment_engine.hpp;models.h"
)

# Main executable
add_executable(slippage
    main.cpp
)

target_link_libraries(slippage PRIVATE slippage_lib)

# Test executable
enable_testing()

include_directories(${CMAKE_SOURCE_DIR}/external/catch2)

add_executable(slippage_tests
    tests/test_assignment.cpp
)

target_link_libraries(slippage_tests PRIVATE slippage_lib)

add_test(NAME SlippageTests COMMAND slippage_tests)

# Install targets
include(GNUInstallDirs)

install(TARGETS slippage slippage_lib
    EXPORT SlippageTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/slippage
)

# Install generated version header
install(FILES
    ${CMAKE_BINARY_DIR}/version.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/slippage
)

# Install csv-parser header for library users
install(FILES
    ${CMAKE_SOURCE_DIR}/external/csv-parser/single_include/csv.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/slippage/external
)

install(FILES
    ${CMAKE_SOURCE_DIR}/ASSIGNMENT_RULES.md
    ${CMAKE_SOURCE_DIR}/README.md
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
)

# Export targets
install(EXPORT SlippageTargets
    FILE SlippageTargets.cmake
    NAMESPACE Slippage::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Slippage
)

# Create config file
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_SOURCE_DIR}/cmake/SlippageConfig.cmake.in
    ${CMAKE_BINARY_DIR}/SlippageConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Slippage
)

write_basic_package_version_file(
    ${CMAKE_BINARY_DIR}/SlippageConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_BINARY_DIR}/SlippageConfig.cmake
    ${CMAKE_BINARY_DIR}/SlippageConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Slippage
)
