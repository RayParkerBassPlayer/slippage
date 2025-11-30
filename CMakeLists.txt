cmake_minimum_required(VERSION 3.10)
project(Slippage VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Configure version header
configure_file(
    ${CMAKE_SOURCE_DIR}/version.h.in
    ${CMAKE_BINARY_DIR}/version.h
)

include_directories(${CMAKE_SOURCE_DIR}/external/csv-parser/single_include)
include_directories(${CMAKE_BINARY_DIR})

# Main executable
add_executable(slippage
    main.cpp
    dimensions.cpp
    slip.cpp
    member.cpp
    assignment.cpp
    csv_parser.cpp
    assignment_engine.cpp
)

# Test executable
enable_testing()

include_directories(${CMAKE_SOURCE_DIR}/external/catch2)

add_executable(slippage_tests
    tests/test_assignment.cpp
    dimensions.cpp
    slip.cpp
    member.cpp
    assignment.cpp
    assignment_engine.cpp
)

add_test(NAME SlippageTests COMMAND slippage_tests)

# Install targets
install(TARGETS slippage
    RUNTIME DESTINATION bin
)

install(FILES
    ${CMAKE_SOURCE_DIR}/ASSIGNMENT_RULES.md
    ${CMAKE_SOURCE_DIR}/README.md
    DESTINATION share/doc/slippage
)
