cmake_minimum_required(VERSION 3.14)
project(vanity VERSION 0.1.0 LANGUAGES CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
add_compile_options(-Wall -Wextra)

# Set output directories to match Makefile structure
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Include directories
include_directories(include)

# Library sources
set(LIB_SOURCES
    src/lib/image_buffer.cpp
    src/lib/image_io.cpp
    src/lib/image_ops.cpp
)

# Library headers
set(LIB_HEADERS
    include/vanity/image_buffer.hpp
    include/vanity/image_io.hpp
    include/vanity/image_ops.hpp
)

# Create static library
add_library(libvanity STATIC ${LIB_SOURCES} ${LIB_HEADERS})
set_target_properties(libvanity PROPERTIES OUTPUT_NAME vanity)
target_include_directories(libvanity PUBLIC include)
target_link_libraries(libvanity PUBLIC m)

# CLI sources
set(CLI_SOURCES
    src/cli/main.cpp
    src/cli/command_registry.cpp
    src/cli/commands.cpp
    src/cli/commands/add_border_command.cpp
)

# Create CLI executable
add_executable(vanity ${CLI_SOURCES})
target_link_libraries(vanity PRIVATE libvanity)

# Testing
option(BUILD_TESTS "Build tests" ON)

if(BUILD_TESTS)
    # FetchContent for Google Test
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    # Enable testing
    enable_testing()

    # Test sources
    set(TEST_SOURCES
        tests/test_image_buffer.cpp
        tests/test_image_io.cpp
        tests/test_image_ops.cpp
    )

    # Create test executable
    add_executable(test_runner ${TEST_SOURCES})
    target_link_libraries(test_runner PRIVATE libvanity gtest_main)

    # Add tests to CTest
    include(GoogleTest)
    gtest_discover_tests(test_runner)
endif()

# Installation
install(TARGETS vanity DESTINATION bin)
install(DIRECTORY include/vanity DESTINATION include)
install(TARGETS libvanity DESTINATION lib)
