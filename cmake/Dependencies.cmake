# External dependencies configuration

include(FetchContent)

# GoogleTest
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/heads/main.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

# Future dependencies can be added here:
# - Boost (for networking)
# - fmt (for logging)
# - spdlog (for logging)
# - benchmark (for performance testing)