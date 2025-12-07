cmake_minimum_required(VERSION 3.10)
project(SlippageExamples)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the installed Slippage library
find_package(Slippage REQUIRED)

# Build the examples
add_executable(example_usage example_usage.cpp)
target_link_libraries(example_usage PRIVATE Slippage::slippage_lib)

add_executable(version_example version_example.cpp)
target_link_libraries(version_example PRIVATE Slippage::slippage_lib)
