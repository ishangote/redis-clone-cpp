# Storage library configuration
add_library(storage STATIC
    src/database.cpp
)

# Include directories
target_include_directories(storage
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Installation rules
install(DIRECTORY include/ 
    DESTINATION include
)

install(TARGETS storage
    EXPORT redis-clone-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
