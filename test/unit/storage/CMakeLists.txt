# Storage tests configuration
add_executable(database_test
    database_test.cpp
)

target_link_libraries(database_test
    PRIVATE
        storage
        GTest::gtest_main
        GTest::gmock
)

include(GoogleTest)
gtest_discover_tests(database_test)