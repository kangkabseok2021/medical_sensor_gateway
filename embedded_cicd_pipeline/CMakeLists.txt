cmake_minimum_required(VERSION 3.21)
project(embedded_pid LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(COVERAGE "Build with coverage support" OFF)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

if(COVERAGE)
    add_compile_options(--coverage -fprofile-arcs -ftest-coverage)
    add_link_options(--coverage -fprofile-arcs -ftest-coverage)
endif()

# Interface library for headers
add_library(ema_headers INTERFACE)
target_include_directories(ema_headers INTERFACE include)

# Static libraries
add_library(pid_lib src/pid/PIDController.cpp)
target_link_libraries(pid_lib PUBLIC ema_headers)

add_library(hal_lib src/hal/SimulatedMotorHAL.cpp)
target_link_libraries(hal_lib PUBLIC ema_headers)

add_library(app_lib src/app/ProcessingLoop.cpp)
target_link_libraries(app_lib PUBLIC pid_lib hal_lib)

# Executable
add_executable(embedded_pid src/main.cpp)
target_link_libraries(embedded_pid PRIVATE app_lib)

# Testing
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(test_pid tests/unit/test_pid_controller.cpp)
target_link_libraries(test_pid PRIVATE pid_lib GTest::gtest_main)
add_test(NAME test_pid COMMAND test_pid)

add_executable(test_hal tests/unit/test_motor_hal.cpp)
target_link_libraries(test_hal PRIVATE hal_lib GTest::gtest_main)
add_test(NAME test_hal COMMAND test_hal)

add_executable(test_integration tests/integration/test_processing_loop.cpp)
target_link_libraries(test_integration PRIVATE app_lib GTest::gtest_main)
add_test(NAME test_integration COMMAND test_integration)
