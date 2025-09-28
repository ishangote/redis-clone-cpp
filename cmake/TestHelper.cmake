# Helper functions for testing

# Function to create a test executable with proper linking and configuration
function(rc_add_test)
    cmake_parse_arguments(TEST "" "NAME;DIRECTORY" "SOURCES;LIBS" ${ARGN})
    
    add_executable(${TEST_NAME} ${TEST_SOURCES})
    target_link_libraries(${TEST_NAME}
        PRIVATE
            ${TEST_LIBS}
            gtest
            gtest_main
            pthread
    )
    
    # Add include directories
    target_include_directories(${TEST_NAME}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src
    )
    
    # Add the test to CTest
    add_test(
        NAME ${TEST_NAME}
        COMMAND ${TEST_NAME}
        WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )
endfunction()