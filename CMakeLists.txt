cmake_minimum_required(VERSION 3.14)
project(redis-clone-cpp VERSION 0.1.0 LANGUAGES CXX)

# Global settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include custom CMake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Only include dependencies and testing when BUILD_TESTING is enabled
if(BUILD_TESTING)
    include(Dependencies)
    include(TestHelper)
    
    # For Windows: prevent overriding parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    
    # Enable testing
    enable_testing()
endif()

# Add main project subdirectories
add_subdirectory(src)

# Only add test directory when testing is enabled
if(BUILD_TESTING)
    add_subdirectory(test)
endif()

# Optional components (uncomment as needed)
# add_subdirectory(examples)
# add_subdirectory(benchmarks)
