# Network library configuration
add_library(network STATIC
    src/server.cpp
    src/redis_utils.cpp
)

target_include_directories(network
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(network
    PUBLIC storage
)