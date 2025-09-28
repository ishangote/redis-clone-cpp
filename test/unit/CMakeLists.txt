# Unit tests configuration

# Storage tests
add_executable(database_test
    storage/database_test.cpp
)

target_link_libraries(database_test
    PRIVATE
        storage
        gtest
        gtest_main
        pthread
)

add_test(NAME database_test COMMAND database_test)