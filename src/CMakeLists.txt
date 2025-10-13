# Main source directory configuration

# Add component subdirectories
add_subdirectory(storage)
add_subdirectory(network)

# Create main executable
add_executable(redis-clone-cpp
    main.cpp
)

# Link against component libraries
target_link_libraries(redis-clone-cpp
    PRIVATE
        storage
        network
        pthread
)

# Future components
# add_subdirectory(replication)
# add_subdirectory(data_structures)
