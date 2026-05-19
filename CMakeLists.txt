cmake_minimum_required(VERSION 3.20)
project(medical_sensor_gateway CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── hal_lib — sensor HAL + signal filter (no FSM, no Linux headers) ──────────
add_library(hal_lib STATIC
    src/FlowSensor.cpp
    src/ECGSensor.cpp
    src/ButterworthFilter.cpp
)
target_include_directories(hal_lib PUBLIC src)
target_compile_options(hal_lib PRIVATE -Wall -Wextra -Wpedantic)

# ── gateway_core — FSM + gateway orchestrator (links hal_lib) ─────────────────
add_library(gateway_core STATIC
    src/MedicalFsm.cpp
    src/AlarmPublisher.cpp
    src/SensorGateway.cpp
)
target_link_libraries(gateway_core PUBLIC hal_lib)
target_compile_options(gateway_core PRIVATE -Wall -Wextra -Wpedantic)

# ── gateway_daemon — main + systemd notify (Linux only) ───────────────────────
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(gateway_daemon src/main.cpp)
    target_link_libraries(gateway_daemon PRIVATE gateway_core)
    target_compile_options(gateway_daemon PRIVATE -Wall -Wextra)

    find_library(SYSTEMD_LIB systemd)
    if(SYSTEMD_LIB)
        target_compile_definitions(gateway_daemon PRIVATE HAVE_SYSTEMD)
        target_link_libraries(gateway_daemon PRIVATE ${SYSTEMD_LIB})
    endif()
endif()

# ── GoogleTest ────────────────────────────────────────────────────────────────
enable_testing()
if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
        URL_HASH SHA256=8ad598c73ad796e0d8280b082cebd82a630d73e73cd3c70057938a6501bba5d7
        DOWNLOAD_EXTRACT_TIMESTAMP NEW
    )
    set(INSTALL_GTEST OFF)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(test_gateway tests/test_gateway.cpp)
    target_link_libraries(test_gateway PRIVATE gateway_core GTest::gtest_main)
    target_compile_options(test_gateway PRIVATE -Wall -Wextra)

    add_test(NAME gateway_tests COMMAND test_gateway)
endif()
