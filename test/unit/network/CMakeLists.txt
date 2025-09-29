# Network tests configuration
add_executable(network_test
    server_test.cpp
)

target_link_libraries(network_test
    PRIVATE
        network
        GTest::gtest_main
        GTest::gmock
)

include(GoogleTest)
gtest_discover_tests(network_test)