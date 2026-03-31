cmake_minimum_required(VERSION 3.16)
project(perimeter_mg LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# LP library
include(FetchContent)
message(STATUS "Fetching HiGHS repository...")
FetchContent_Declare(
    highs
    GIT_REPOSITORY https://github.com/ERGO-Code/HiGHS.git
    GIT_TAG v1.13.1
)
FetchContent_MakeAvailable(highs)

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
    src/learning/nash_q_learning.cpp
    src/learning/single_agent_simple_game_policy.cpp
    src/learning/joint.cpp
)
target_link_libraries(perimeter_geometry
    PUBLIC
        highs
)
target_include_directories(perimeter_geometry
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(${PROJECT_NAME}
    src/main.cpp)
target_link_libraries(${PROJECT_NAME} perimeter_geometry)

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
    tests/single_agent_simple_game_policy_test.cpp
    tests/joint_test.cpp
    tests/nash_q_learning_test.cpp
)

target_link_libraries(geometry_tests
    PRIVATE
        perimeter_geometry
        ${GTEST_MAIN_TARGET}
)

include(GoogleTest)
gtest_discover_tests(geometry_tests)
