cmake_minimum_required(VERSION 3.16)
project(perimeter_mg LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(perimeter_geometry
    src/geometry/Hex.cpp
    src/geometry/HexGrid.cpp
    src/core/WorldState.cpp
    src/environment/Movement.cpp
    src/environment/Transition.cpp
    src/environment/Simulator.cpp
    src/environment/Initialization.cpp
    src/visualization/AsciiViewer.cpp
    src/visualization/JsonViewer.cpp
)

target_include_directories(perimeter_geometry
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

enable_testing()

find_package(GTest QUIET)
if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    FetchContent_MakeAvailable(googletest)
endif()

if(TARGET GTest::gtest_main)
    set(GTEST_MAIN_TARGET GTest::gtest_main)
else()
    set(GTEST_MAIN_TARGET gtest_main)
endif()

add_executable(geometry_tests
    tests/hex_geometry_test.cpp
    tests/world_state_test.cpp
    tests/movement_test.cpp
    tests/transition_test.cpp
    tests/initialization_test.cpp
    tests/simulator_test.cpp
    tests/agent_interface_test.cpp
    tests/visualization_test.cpp
)

target_link_libraries(geometry_tests
    PRIVATE
        perimeter_geometry
        ${GTEST_MAIN_TARGET}
)

include(GoogleTest)
gtest_discover_tests(geometry_tests)
