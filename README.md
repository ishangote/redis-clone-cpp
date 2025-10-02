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
- **Storage Engine**: Thread-safe key-value storage implementation
  - In-memory key-value store using std::unordered_map
  - String values support
  - GET/SET/DEL/EXISTS operations
  - Thread-safe database operations with mutex protection
- **Network Layer**: Multi-threaded TCP server with robust command processing
  - **Multi-threading**: Concurrent client connections (one thread per client)
  - **Buffer Management**: NEW - Robust handling of partial command reception across multiple recv() calls
  - **Command Processing**: NEW - Complete command accumulation until proper terminators (\r\n or \n)
  - **Thread Safety**: Mutex-protected database operations for concurrent access
  - **Signal Handling**: Proper signal blocking in worker threads for graceful shutdown
  - **Connection Management**: Persistent client connections with proper cleanup on disconnection
  - **Error Handling**: Robust error handling for network failures and client disconnections
  - **Redis Protocol**: Basic RESP (Redis Serialization Protocol) response formatting

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

The server supports **multiple concurrent clients** with thread-safe operations and **NEW: production-quality buffer management**. Recent improvements include:

- **Buffer Management**: NEW - Handles partial commands that don't arrive in a single network packet
- **Command Boundary Detection**: NEW - Properly detects complete commands using \r\n or \n terminators
- **Partial Command Accumulation**: NEW - Accumulates command data across multiple recv() calls until complete

Existing features:
- **Concurrent Connections**: Each client runs in its own thread, allowing unlimited simultaneous connections
- **Thread Safety**: All database operations are protected with mutexes to prevent race conditions
- **Graceful Disconnection**: Proper cleanup when clients disconnect unexpectedly
- **Signal Handling**: Worker threads block signals to ensure proper shutdown behavior

Commands can be sent independently by each client without affecting others:

## Architecture & Design

### Multi-threaded Server Architecture
The server implements a **one-thread-per-client** model with the following design:

```
Main Thread                    Worker Threads
     |                              |
┌────▼────┐                  ┌─────▼─────┐
│ Accept  │                  │  Client   │
│ Loop    │────────────────▶ │  Handler  │
│         │                  │  Thread   │
└─────────┘                  └───────────┘
                                    │
                               ┌────▼────┐
                               │ Command │
                               │ Buffer  │◄─── recv() calls
                               │ Manager │
                               └─────────┘
                                    │
                               ┌────▼────┐
                               │Database │◄─── Mutex protected
                               │Operations│
                               └─────────┘
```

### Buffer Management (NEW)
A critical networking challenge recently solved in this implementation:

**Problem**: TCP doesn't guarantee that a complete command arrives in a single `recv()` call. Large commands or network conditions can cause partial reception, leading to parsing failures.

**Example**: Command `"SET username john_doe_with_a_long_name"` might arrive as:
- recv() call 1: `"SET username john_doe_with_a_lo"`
- recv() call 2: `"ng_name"`

**Solution**: Command buffer accumulation with terminator detection:
1. **Persistent Buffer**: Maintain `std::string command_buffer` per client connection
2. **Data Accumulation**: Append all recv() data to buffer using `command_buffer.append()`
3. **Boundary Detection**: Scan for complete commands ending with \n or \r\n using `find('\n')`
4. **Command Extraction**: Extract complete commands with `substr()` and remove with `erase()`
5. **Partial Preservation**: Keep incomplete command data for next recv() cycle

**Testing**: Verified by reducing buffer size to 30 bytes to simulate partial reception scenarios.

### Thread Safety Model
- **Database Layer**: Protected by `std::mutex` with RAII (`std::lock_guard`)
- **Worker Threads**: Independent per-client threads with signal blocking
- **Main Thread**: Handles accept() loop and signal management
- **Memory Safety**: Automatic cleanup on client disconnection

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
1. ✅ **Basic Redis Commands**
   - GET/SET/DEL/EXISTS operations
   - Basic string value support
   - Error handling and validation
2. ✅ **Networking**
   - Multi-threaded TCP server implementation
   - Concurrent client connection handling
   - Basic Redis protocol (RESP) response formatting
   - NEW: Production-quality buffer management for partial commands
   - Thread-safe client-server communication
3. 🚧 **Advanced Features** (Planned)
   - Full Redis protocol (RESP) parsing
   - Data structure operations (Lists, Sets, Hashes)
   - Replication and clustering
   - Persistence (RDB snapshots, AOF)

## Contributing

This is a learning project but contributions are welcome! Please feel free to:
- Report bugs
- Suggest features
- Submit pull requests

## Acknowledgments

- Redis for inspiration
