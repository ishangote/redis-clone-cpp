# Redis Clone in C++

A from-scratch implementation of Redis to understand distributed systems concepts, in-memory databases, and concurrent programming fundamentals.

## Project Overview

This project aims to recreate Redis's core functionality to gain deep insights into:
- In-memory database systems
- Distributed system concepts
- Network programming
- Concurrent data structures
- High-performance I/O

## Project Structure

```
redis-clone-cpp/
├── CMakeLists.txt          # Top-level CMake configuration
├── cmake/                  # CMake modules and utilities
│   ├── Dependencies.cmake  # External dependency management
│   └── TestHelper.cmake   # Test configuration helpers
├── src/                   # Source code
│   ├── CMakeLists.txt    # Source build configuration
│   ├── storage/          # Storage engine implementation
│   │   ├── CMakeLists.txt
│   │   ├── include/     # Public headers
│   │   │   └── storage/
│   │   │       └── database.h
│   │   └── src/        # Implementation files
│   │       └── database.cpp
│   └── network/         # Network server implementation
│       ├── CMakeLists.txt
│       ├── include/     # Public headers
│       │   └── network/
│       │       └── server.h
│       └── src/        # Implementation files
│           └── server.cpp
├── test/                # Test files
│   ├── CMakeLists.txt  # Test configuration
│   └── unit/           # Unit tests
│       ├── storage/    # Storage engine tests
│       │   ├── CMakeLists.txt
│       │   └── database_test.cpp
│       └── network/    # Network component tests
│           ├── CMakeLists.txt
│           └── server_test.cpp
└── scripts/            # Build and test scripts
    ├── build.sh       # Build script
    └── test.sh        # Test runner script
```

## Components

### Current Implementation
- **Storage Engine**: Basic key-value storage implementation
  - In-memory key-value store
  - String values support
  - GET/SET/DEL/EXISTS operations
  - Thread-safe database operations with mutex protection
- **Network Layer**: Multi-threaded TCP server implementation
  - Concurrent client connections (one thread per client)
  - Persistent client connections with proper cleanup
  - Basic command handling (GET/SET/DEL/EXISTS/QUIT)
  - Signal handling for graceful shutdown
  - Structured command parsing
  - Robust error handling for client disconnections

### Planned Components
- **Data Structures**: Redis-like data structure support
- **Replication**: Master-slave replication system
- **Persistence**: Snapshot and AOF persistence

## Build System

The project uses CMake as its build system with a modern setup:

### CMake Structure
- **Top-level CMake**: Project-wide settings and component inclusion
- **Component-level CMake**: Individual component configurations
- **Modern CMake Practices**: 
  - Target-based configuration
  - Proper dependency management
  - Clear public/private interfaces

### Build Configuration
- C++17 standard
- GoogleTest for unit testing
- Modular component architecture
- Separate build outputs for binaries and libraries

## Getting Started

### Prerequisites
- CMake 3.14 or higher
- C++17 compatible compiler
- Git

### Building the Project
```bash
# Clone the repository
git clone https://github.com/yourusername/redis-clone-cpp.git
cd redis-clone-cpp

# Build the project
./scripts/build.sh
```

### Running Tests
```bash
# Run all tests
./scripts/test.sh

# Run specific test components
./scripts/test.sh network_test     # Run network tests
./scripts/test.sh database_test    # Run storage tests

# Run tests without rebuilding
./scripts/test.sh --no-build network_test
```

### Running the Server
```bash
# Build and start the server (default port 6379)
./scripts/build.sh
./build/bin/redis-clone-cpp

# Or specify a custom port
./build/bin/redis-clone-cpp 8080

# Or use environment variable
REDIS_CLONE_PORT=9000 ./build/bin/redis-clone-cpp
```

### Testing with netcat
Once the server is running, you can test it manually using netcat:

```bash
# Connect multiple clients simultaneously
# Terminal 1:
nc localhost 6379

# Terminal 2 (concurrent connection):
nc localhost 6379

# Terminal 3 (another concurrent connection):
nc localhost 6379

# Each client can send commands independently:
SET name john
GET name
SET age 25
GET age
DEL name
EXISTS name
EXISTS age
QUIT    # Gracefully close the connection
```

The server now supports **multiple concurrent clients** with thread-safe operations. Each client runs in its own thread and can send commands independently without affecting other clients.

## Development Workflow

### Adding New Components
1. Create component directory in `src/`
2. Add component's CMakeLists.txt
3. Update top-level CMakeLists.txt
4. Add corresponding test directory
5. Update test CMakeLists.txt

### Testing
- Unit tests using GoogleTest framework
- Tests located in `test/unit/`
- Each component has its own test directory
- Test helper functions in `cmake/TestHelper.cmake`

## Project Goals

### Educational Objectives
- Understanding distributed systems concepts
- Learning concurrent programming patterns
- Implementing high-performance data structures
- Network programming fundamentals
- Database system internals

### Technical Goals
1. Basic Redis Commands
   - GET/SET operations
   - Data structure operations
   - Basic query support
2. Networking
   - TCP server implementation
   - Redis protocol (RESP) support
   - Client-server communication
3. Advanced Features
   - Replication
   - Persistence
   - Cluster support

## Contributing

This is a learning project but contributions are welcome! Please feel free to:
- Report bugs
- Suggest features
- Submit pull requests

## Acknowledgments

- Redis for inspiration
