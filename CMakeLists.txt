# ===========================================================================
# UAV Telemetry Ingester Project Build Configuration
# ===========================================================================
cmake_minimum_required(VERSION 3.20)
project(uav_telemetry_ingester CXX)

# Enforce the C++20 language specification across all project components
# (Required for designated initializers and explicit template lambdas)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Locate and expose the host system's threading framework (POSIX pthreads)
find_package(Threads REQUIRED)

# ---------------------------------------------------------------------------
# Target 1: Main Telemetry Ingester Executable
# ---------------------------------------------------------------------------
add_executable(uav_telemetry_ingester
        main.cpp
        src/sensors/FlightController.cpp
        src/sensors/Gps.cpp
        src/sensors/Imu.cpp
        src/sensors/Battery.cpp
        src/network/UdpReceiver.cpp
        src/engine/IngestionEngine.cpp
        src/display/TerminalRenderer.cpp
        src/packets/WirePackets.h
)

# Apply explicit code quality flags strictly to our own target
target_compile_options(uav_telemetry_ingester PRIVATE -Wall -Wextra -Wpedantic)

# Expose the project root folder for relative inclusion paths (e.g. #include "src/...")
target_include_directories(uav_telemetry_ingester PRIVATE ${CMAKE_SOURCE_DIR})

# Bind low-level operating system threads to the ingestion runner pipeline
target_link_libraries(uav_telemetry_ingester PRIVATE Threads::Threads)

# ---------------------------------------------------------------------------
# Target 2: UAV Flight Simulator Executable (Testing Tool)
# ---------------------------------------------------------------------------
add_executable(uav_simulator
        simulator/main.cpp
        simulator/Telemetry.cpp
        simulator/SimulationState.h
)

# Apply explicit code quality flags strictly to our own target
target_compile_options(uav_simulator PRIVATE -Wall -Wextra -Wpedantic)

# Sandbox target isolation: declare explicit include paths for the simulator target
target_include_directories(uav_simulator PRIVATE ${CMAKE_SOURCE_DIR})

# Link threading capabilities to manage high-frequency simulation timers (50 Hz, 10 Hz)
target_link_libraries(uav_simulator PRIVATE Threads::Threads)

# ---------------------------------------------------------------------------
# Target 3: Automated Unit Test Suite (GoogleTest Framework)
# ---------------------------------------------------------------------------
include(CTest) # Declares the standard BUILD_TESTING option flag (Defaults to ON)

if(BUILD_TESTING)
    include(FetchContent)

    # Dynamically download and configure GoogleTest during the CMake generate pass
    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG        v1.15.2
    )

    # Optimization: Prevent testing binaries from copying out to system directory targets
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    # Group shared functional sensor dependency objects into a reusable macro collection
    set(SENSOR_SOURCES
            src/sensors/FlightController.cpp
            src/sensors/Gps.cpp
            src/sensors/Imu.cpp
            src/sensors/Battery.cpp
            src/packets/WirePackets.h
    )

    # Define the automated testing target runner binary
    add_executable(uav_tests
            tests/test_battery.cpp
            tests/test_packet_layout.cpp
            tests/test_sensors.cpp
            ${SENSOR_SOURCES}
    )

    # Apply explicit code quality flags strictly to our own test runner target
    target_compile_options(uav_tests PRIVATE -Wall -Wextra -Wpedantic)

    # Provide the test suite search framework access to core application headers
    target_include_directories(uav_tests PRIVATE ${CMAKE_SOURCE_DIR})

    # Link the downloaded GoogleTest runtime entry-point and system thread controls
    target_link_libraries(uav_tests PRIVATE gtest_main Threads::Threads)

    # Invoke testing suite macros to automatically discover and register individual assertions
    include(GoogleTest)
    gtest_discover_tests(uav_tests)
endif()
