# Redis Clone in C++

A from-scratch implementation of Redis featuring **dual server architectures** and **production-grade persistence** for comprehensive distributed systems learning. This project demonstrates event-driven programming, multi-threaded design, and complete Redis-style data persistence with both RDB snapshots and AOF (Append-Only File) logging.

## Project Overview

This project recreates Redis's core functionality with **two distinct server architectures** and **complete persistence support** to provide deep insights into:
- **Event-driven programming** with I/O multiplexing (select/poll)
- **Multi-threaded server design** with synchronization challenges  
- **Redis-style persistence** with RDB snapshots and AOF logging
- **Write-ahead logging** with configurable fsync policies
- **Background operations** with fork() and copy-on-write semantics
- **Distributed system concepts** and concurrent programming patterns
- **Network programming** and protocol implementation
- **High-performance I/O** and memory management

### Key Educational Features
- **Side-by-side comparison** of event-driven vs multi-threaded architectures
- **Complete persistence system** with RDB snapshots and AOF logging
- **Automatic AOF rewriting** with size-based compaction
- **Redis-style recovery** with AOF-first precedence
- **Process forking** for zero-downtime background operations (BGSAVE, BGREWRITEAOF)
- **Write-ahead logging** with configurable durability policies
- **Command-line mode selection** for easy switching between implementations
- **Production-quality** buffer management and protocol handling
- **Comprehensive documentation** of design decisions and trade-offs

## Project Structure

```
redis-clone-cpp/
├── CMakeLists.txt          # Top-level CMake configuration
├── cmake/                  # CMake modules and utilities
│   ├── Dependencies.cmake  # External dependency management
│   └── TestHelper.cmake   # Test configuration helpers
├── src/                   # Source code
│   ├── main.cpp          # Entry point with command-line parsing
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
│       │       ├── server.h      # Both server architectures
│       │       └── redis_utils.h # Shared protocol utilities
│       └── src/        # Implementation files
│           ├── server.cpp      # Server implementations
│           └── redis_utils.cpp # Shared utilities
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

## Server Architectures

### 🚀 Event-Driven Server (Default) - `RedisServer`
**Production-style architecture** with persistence support using I/O multiplexing:

- **Single-threaded** event loop with `select()` system call
- **Non-blocking I/O** operations (MSG_DONTWAIT)
- **Client state management** with per-client read/write buffers
- **Redis-style persistence** with automatic and background saves
- **Process forking** for zero-downtime BGSAVE operations
- **Memory efficient** - no thread overhead per connection
- **Scalable** to thousands of concurrent connections

**Best for**: Learning modern server architectures, understanding event loops, Redis persistence concepts, and high-concurrency scenarios.

### 🧵 Multi-Threaded Server (Educational) - `ThreadedRedisServer`
**Traditional thread-per-client architecture** for understanding threading concepts:

- **One thread per client** connection model
- **Thread-safe database** operations with mutex synchronization
- **Signal handling** in worker threads
- **Resource isolation** per client connection
- **Simpler logic** but higher memory overhead
- **Storage layer abstraction** for educational comparison

**Best for**: Understanding threading, synchronization challenges, and resource management.

## Components

### Current Implementation
- **Dual Server Architectures**: 
  - **Event-driven** (default): Single-threaded with I/O multiplexing using `select()` and persistence support
  - **Multi-threaded** (educational): Thread-per-client with mutex synchronization and storage abstraction
  
- **Redis-Style Persistence**:
  - **RDB snapshots**: Background saves triggered by Redis-style conditions (900s+1, 300s+10, 60s+10k)
  - **AOF logging**: Write-ahead logging with configurable fsync policies (always, everysec, no)
  - **BGSAVE command**: Manual background RDB saves using fork() for zero-downtime operation
  - **BGREWRITEAOF command**: Background AOF compaction to prevent file bloat
  - **Automatic AOF rewriting**: Size-based triggers (2x growth, 64MB minimum)
  - **Redis-style recovery**: AOF-first precedence with RDB fallback
  - **Startup recovery**: Automatic data loading on server restart
  - **Atomic file operations**: Temporary file + rename for crash safety
  - **Process management**: SIGCHLD handling for background operation cleanup

- **Command-Line Interface**:
  - **Mode selection**: `--mode=eventloop|threaded` 
  - **Port configuration**: `--port=<number>` (default: 6379)
  - **Help system**: `-h, --help` for usage information
  - **Backward compatibility**: Supports legacy positional arguments

- **Storage Engine**: Thread-safe key-value storage implementation
  - In-memory key-value store using `std::unordered_map`
  - String values support with GET/SET/DEL/EXISTS operations
  - Thread-safe database operations (multi-threaded mode)
  - Template-based storage abstraction (event-loop mode)
  - Persistence change tracking for automatic save triggers

- **Network Layer**: Production-quality TCP server with robust protocol handling
  - **Redis Protocol (RESP)**: Complete command parsing and response formatting
  - **Buffer Management**: Handles partial commands across multiple `recv()` calls
  - **Command Processing**: Flexible termination handling (`\r\n` and `\n`)
  - **Connection Management**: Graceful client disconnection and cleanup
  - **Error Handling**: Comprehensive error responses and network failure recovery
  - **BGSAVE Integration**: Non-blocking background save command support

- **Shared Utilities**: `redis_utils` module for protocol consistency
  - **Command parsing**: Structured command extraction (`CommandParts`)
  - **Template specialization**: Support for different storage backends
  - **Protocol compliance**: RESP format responses for all commands
  - **Code reuse**: Shared logic between both server architectures

### Planned Components
- **Additional Data Structures**: Redis-like list, set, and hash support
- **Persistence Configuration**: Configurable save conditions, AOF policies, and file paths
- **Replication**: Master-slave replication system
- **Advanced Features**: TTL support, key expiration, memory optimization

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

#### Quick Start
```bash
# Build the project
./scripts/build.sh

# Start with default settings (Event-driven server on port 6379)
./build/bin/redis-clone-cpp

# Expected output:
# Redis Clone Server v0.1.0
# Mode: Event Loop
# Port: 6379
# PID: 12345
# --------------------------------
# Event loop server ready to accept connections
```

#### Server Mode Selection
```bash
# Event-driven server (default) - recommended for learning modern architectures
./build/bin/redis-clone-cpp --mode=eventloop

# Multi-threaded server - educational comparison
./build/bin/redis-clone-cpp --mode=threaded

# Custom port
./build/bin/redis-clone-cpp --mode=eventloop --port=8080

# Using environment variable
REDIS_CLONE_PORT=9000 ./build/bin/redis-clone-cpp --mode=threaded
```

#### Help and Usage
```bash
# Display all options
./build/bin/redis-clone-cpp --help

# Output:
# Usage: ./build/bin/redis-clone-cpp [OPTIONS]
# Options:
#   --mode=<type>     Server mode: 'eventloop' (default) or 'threaded'
#   --port=<number>   Port number (default: 6379)
#   -h, --help        Show this help message
#
# Examples:
#   ./build/bin/redis-clone-cpp                    # Event loop server on port 6379
#   ./build/bin/redis-clone-cpp --mode=threaded    # Multi-threaded server
#   ./build/bin/redis-clone-cpp --port=8080        # Custom port
```

### Testing with netcat

Once the server is running, you can test it using netcat:

```bash
# Connect to server
nc localhost 6379

# Send commands manually:
SET name john
GET name
SET age 25
GET age
DEL name
EXISTS name
EXISTS age
BGSAVE    # Trigger background save
QUIT      # Gracefully close the connection
```

#### Testing Persistence Features

Test the complete persistence functionality to see Redis-style data durability:

```bash
# Terminal 1: Start server
./build/bin/redis-clone-cpp

# Terminal 2: Connect and add data
nc localhost 6379
SET user:1 alice
SET user:2 bob
SET counter 42
BGSAVE              # Manual RDB snapshot
# Observe: "Background save started (PID: xxxxx)" message
# Wait for: "Background save completed" message

# Test AOF logging (all write commands are logged)
SET session:123 active
DEL user:2
SET temp data

# Test AOF rewrite
BGREWRITEAOF        # Manual AOF compaction
# Observe: "AOF rewrite started (PID: xxxxx)" message

QUIT

# Terminal 1: Stop server (Ctrl+C)
# Terminal 1: Restart server
./build/bin/redis-clone-cpp
# Observe: "Loading data from AOF file..." (AOF takes precedence)
# Or: "Loading data from RDB snapshot..." (if no AOF available)

# Terminal 2: Verify data persistence
nc localhost 6379
GET user:1          # Should return "alice"
GET session:123     # Should return "active"
EXISTS user:2       # Should return "0" (was deleted)
GET counter         # Should return "42"

# Test automatic AOF rewriting by making many changes:
SET test1 value1
SET test2 value2
# ... continue adding keys to trigger size-based rewrite
# Watch for "AOF file grew too large, triggering background rewrite..." message
```

#### Testing Multiple Concurrent Connections
You can test both server architectures with multiple simultaneous connections:

```bash
# Terminal 1: Start event-driven server
./build/bin/redis-clone-cpp --mode=eventloop

# Terminal 2-4: Connect multiple clients simultaneously
nc localhost 6379  # Each terminal can send commands independently

# Example commands in each terminal:
# Terminal 2: SET user1 alice
# Terminal 3: SET user2 bob  
# Terminal 4: GET user1

# Then test multi-threaded mode:
./build/bin/redis-clone-cpp --mode=threaded
# Connect same number of clients and observe behavior
```

## Architecture Comparison & Design

### Event-Driven vs Multi-Threaded: A Learning Comparison

Both server architectures implement the same Redis protocol but use fundamentally different approaches to concurrency:

| Aspect | Event-Driven (`eventloop`) | Multi-Threaded (`threaded`) |
|--------|------------------------------|------------------------------|
| **Concurrency Model** | Single thread + I/O multiplexing | One thread per client |
| **Resource Usage** | Low memory, single thread | Higher memory, N threads |
| **Scalability** | Handles thousands of clients | Limited by thread overhead |
| **Complexity** | Complex state management | Simpler per-client logic |
| **Debugging** | Single-threaded debugging | Multi-threaded race conditions |
| **Persistence** | Full Redis-style persistence | Storage layer abstraction only |
| **Best Use Case** | High-concurrency production | Educational/simple scenarios |

### Event-Driven Server Architecture (`RedisServer`)

```
Event Loop (Single Thread) + Persistence
┌─────────────────────────────────────────────────────────────┐
│                    select() System Call                     │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │
│  │   Client    │  │   Client    │  │   Client    │          │
│  │   Socket    │  │   Socket    │  │   Socket    │   ...    │
│  │     #3      │  │     #4      │  │     #5      │          │
│  └─────────────┘  └─────────────┘  └─────────────┘          │
│         │                 │                 │               │
│  ┌──────▼──────┐  ┌─────▼─────┐   ┌─────▼─────┐             │
│  │ Read Buffer │  │Read Buffer│   │Read Buffer│             │
│  │Write Buffer │  │Write Buffer│  │Write Buffer│             │
│  └─────────────┘  └───────────┘   └───────────┘             │
│                                                              │
│  ┌─────────────────────────────────────────────────────────┐│
│  │             Shared Data Store + Persistence             ││
│  │         std::unordered_map<string, string>              ││
│  │    Changes: 0   Last Save: T-300s   Conditions:        ││
│  │    900s+1 | 300s+10 | 60s+10k    ──► Background Save   ││
│  └─────────────────────────────────────────────────────────┘│
│                                                              │
│  ┌─────────────────────────────────────────────────────────┐│
│  │                 BGSAVE Process                          ││
│  │    fork() ──► Child Process ──► save_snapshot()         ││
│  │    Parent continues serving    ──► SIGCHLD cleanup      ││
│  └─────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
```

**Key Benefits:**
- **Memory Efficient**: No thread stack overhead per client
- **Cache Friendly**: Single thread uses CPU cache effectively  
- **No Locks Needed**: No synchronization primitives required
- **Predictable Performance**: No context switching overhead
- **Redis-Style Persistence**: Automatic and manual background saves
- **Zero-Downtime Saves**: fork() enables non-blocking persistence

### Multi-Threaded Server Architecture (`ThreadedRedisServer`)

```
Main Thread                     Worker Threads
     │                               │
┌────▼────┐                   ┌──────▼──────┐
│ Accept  │                   │   Client    │
│  Loop   │─────────────────▶ │   Handler   │
│         │                   │   Thread    │
└─────────┘                   └─────────────┘
                                     │
                              ┌──────▼──────┐
                              │   Command   │
                              │   Buffer    │◄─── recv() calls
                              │   Manager   │
                              └─────────────┘
                                     │
                              ┌──────▼──────┐
                              │  Database   │◄─── Mutex protected
                              │   Layer     │     std::lock_guard
                              │ (Storage)   │     (Educational)
                              └─────────────┘
```

**Key Benefits:**
- **Simple Logic**: Each client handled independently
- **Isolation**: Client failures don't affect others
- **Familiar Model**: Traditional threading approach
- **Educational Value**: Demonstrates synchronization challenges
- **Storage Abstraction**: Clean separation of networking and storage layers

## Redis-Style Persistence Implementation

A comprehensive persistence system that mirrors Redis's dual persistence approach with both RDB snapshots and AOF (Append-Only File) logging:

### Persistence Features

#### 1. **RDB Snapshots (Point-in-Time)**
Redis-style save conditions trigger automatic snapshots:
```cpp
// Automatic save conditions (matching Redis defaults):
// save 900 1     - Save if ≥1 change in 900 seconds (15 min)  
// save 300 10    - Save if ≥10 changes in 300 seconds (5 min)
// save 60 10000  - Save if ≥10,000 changes in 60 seconds (1 min)
```

#### 2. **AOF Logging (Write-Ahead Log)**
Every write command is logged to ensure durability:
```cpp
// All write operations logged to appendonly.aof:
SET user:1 alice    # Logged immediately
DEL temp           # Logged immediately  
SET counter 42     # Logged immediately

// Configurable fsync policies:
// ALWAYS  - fsync after every write (maximum durability)
// EVERYSEC - fsync every second (Redis default)
// NO      - Let OS decide when to fsync (maximum performance)
```

#### 3. **Background Operations (Zero-Downtime)**
```bash
# Manual RDB snapshot
BGSAVE
# Response: "+Background saving started"

# Manual AOF compaction  
BGREWRITEAOF
# Response: "+Background AOF rewrite started"
```

#### 4. **Automatic AOF Rewriting**
Size-based triggers prevent AOF file bloat:
```cpp
// Auto-rewrite conditions:
// - AOF file grows to 2x size after last rewrite
// - AND AOF file is at least 64MB
// - Checked every 100 write operations

// Example: AOF starts at 10MB → grows to 20MB → auto-rewrite triggered
```

#### 5. **Redis-Style Recovery**
Smart recovery prioritizes AOF over RDB:
```cpp
// Recovery logic (matches Redis behavior):
if (aof_enabled && aof_file_exists()) {
    load_aof_from_file();  // AOF takes precedence (more complete)
} else if (rdb_file_exists()) {
    load_rdb_from_file();  // RDB fallback
} else {
    // Start with empty database
}
```

#### 6. **Process Forking for Zero-Downtime**
```cpp
pid_t pid = fork();
if (pid == 0) {
    // Child process: save snapshot and exit
    save_snapshot_to_file();
    exit(0);
} else if (pid > 0) {
    // Parent process: continue serving clients
    return "+Background saving started\r\n";
}
```

**Benefits of fork() approach:**
- **Zero downtime**: Server continues accepting connections
- **Copy-on-write**: Child gets snapshot of data at fork time
- **Memory efficient**: OS handles memory sharing automatically
- **Crash safety**: Child process failure doesn't affect server

#### 7. **Atomic File Operations**
```cpp
// Write to temporary file first
std::ofstream file("data/dump.json.tmp");
// ... write JSON data ...
file.close();

// Atomic rename (crash-safe)
std::rename("data/dump.json.tmp", "data/dump.json");
```

#### 8. **Startup Recovery**
```cpp
// Server constructor automatically loads existing snapshot
load_snapshot_from_file();
// Output: "Loaded X keys from snapshot"
```

### File Structure
```
data/
├── dump.json          # RDB snapshot (JSON format)
├── dump.json.tmp      # Temporary file during RDB saves
├── appendonly.aof     # AOF log file
└── appendonly.aof.tmp # Temporary file during AOF rewrites
```

### Persistence Formats

#### RDB Snapshot Format (JSON)
Human-readable JSON format with metadata:
```json
{
  "metadata": {
    "version": "1.0",
    "timestamp": "2025-10-10T15:30:45Z",
    "key_count": 3
  },
  "data": {
    "user:1": "alice", 
    "user:2": "bob",
    "counter": "42"
  }
}
```

#### AOF Log Format
Redis-compatible command logging:
```
SET user:1 alice
SET user:2 bob
DEL temp
SET counter 42
```

### Signal Handling and Process Management

Robust child process cleanup prevents zombie processes:
```cpp
// SIGCHLD handler reaps background save processes
void handle_child_exit(int sig) {
    int status;
    pid_t pid;
    
    // Non-blocking wait for all finished children
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status)) {
            std::cout << "Background save completed (PID: " << pid << ")" << std::endl;
        }
    }
}
```

### Change Tracking

Persistence triggers are based on successful write operations:
```cpp
// Count changes for automatic save conditions
if ((response[0] == '+' && response.substr(0, 3) == "+OK") ||
    (response[0] == ':' && response[1] == '1')) {
    if (parts.command == "SET" || parts.command == "DEL") {
        changes_since_save++;
    }
}
```

### File Structure
```
data/
├── dump.json      # Current snapshot
└── dump.json.tmp  # Temporary file during saves (atomic operation)
```

Both architectures use the same protocol handling code for consistency:

```cpp
// Shared command parsing
struct CommandParts {
    std::string command;  // "SET", "GET", etc.
    std::string key;      // Key name
    std::string value;    // Value (for SET)
};

// Template-based processing for different storage types
template<typename DataStore>
std::string process_command_with_store(const CommandParts& parts, DataStore& data);
```

### Shared Protocol Implementation (`redis_utils`)

Both architectures use the same protocol handling code for consistency:

```cpp
// Shared command parsing
struct CommandParts {
    std::string command;  // "SET", "GET", etc.
    std::string key;      // Key name
    std::string value;    // Value (for SET)
};

// Template-based processing for different storage types
template<typename DataStore>
std::string process_command_with_store(const CommandParts& parts, DataStore& data);
```

This design enables:
- **Code Reuse**: Same protocol logic across architectures
- **Consistency**: Identical command behavior in both modes
- **Maintainability**: Single source of truth for Redis protocol
- **Testing**: Compare architectures with identical functionality

### Production-Quality Buffer Management

A critical networking challenge solved in both server implementations:

**The Problem**: TCP doesn't guarantee that complete commands arrive in a single `recv()` call. Network conditions and command size can cause **partial command reception**.

**Example Scenario**:
```bash
# Large command might arrive fragmented:
Command: "SET user:12345:profile:data some_very_long_value_that_spans_multiple_packets"

# recv() call 1: "SET user:12345:profile:data some_very_lo"
# recv() call 2: "ng_value_that_spans_multiple_packets"
```

**Robust Solution Implemented**:
1. **Persistent Buffers**: Maintain `std::string read_buffer` per client
2. **Data Accumulation**: Append all `recv()` data: `read_buffer.append(buffer, bytes_read)`
3. **Boundary Detection**: Scan for complete commands: `buffer.find('\n')`  
4. **Command Extraction**: Extract complete commands and preserve partial data
5. **Flexible Terminators**: Handle both `\r\n` (Redis standard) and `\n` (telnet/netcat)

**Code Example**:
```cpp
// Both architectures use this pattern
while ((pos = read_buffer.find('\n')) != std::string::npos) {
    std::string complete_command = read_buffer.substr(0, pos);
    read_buffer.erase(0, pos + 1);  // Remove processed command
    
    // Remove \r if present (\r\n handling)
    if (!complete_command.empty() && complete_command.back() == '\r') {
        complete_command.pop_back();
    }
    
    // Process complete command
    process_command(complete_command);
}
// Partial commands remain in buffer for next recv() cycle
```

**Testing Verification**: Simulated partial reception by reducing buffer sizes to force command fragmentation across multiple network calls.

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

1. ✅ **Dual Server Architectures**
   - Event-driven server with I/O multiplexing (`select()`)
   - Multi-threaded server with thread-per-client model
   - Command-line mode selection and configuration
   - Production-quality buffer management for both architectures

2. ✅ **Redis Protocol Implementation**
   - RESP (Redis Serialization Protocol) compliance
   - GET/SET/DEL/EXISTS operations with proper error handling
   - Flexible command termination (`\r\n` and `\n`)
   - Shared protocol utilities for code consistency

3. ✅ **Production-Quality Networking**
   - Robust partial command handling across multiple `recv()` calls
   - Graceful client connection management and cleanup
   - Signal handling and graceful shutdown procedures
   - Thread-safe database operations (multi-threaded mode)

4. ✅ **Complete Redis-Style Persistence**
   - RDB snapshots with automatic background saves (Redis-style conditions: 900s+1, 300s+10, 60s+10k)
   - AOF (Append-Only File) logging with write-ahead logging for maximum durability
   - Configurable fsync policies (ALWAYS, EVERYSEC, NO) for durability vs performance trade-offs
   - Manual BGSAVE and BGREWRITEAOF commands using fork() for zero-downtime operations
   - Automatic AOF rewriting with size-based triggers (2x growth, 64MB minimum)
   - Redis-style recovery logic with AOF-first precedence and RDB fallback
   - Atomic file operations with temporary files for crash safety
   - Startup recovery with automatic data loading from persistence files
   - SIGCHLD handling for background process cleanup and zombie prevention

5. 🚧 **Advanced Features** (Planned)
   - Additional Redis commands (INCR, DECR, APPEND, etc.)
   - Data structure operations (Lists, Sets, Hashes)
   - Persistence configuration system for runtime settings
   - Basic replication (master-slave)
   - TTL support and key expiration

## Learning Outcomes

This project provides hands-on experience with:

### Distributed Systems Concepts
- **Concurrency Models**: Event-driven vs multi-threaded approaches
- **I/O Multiplexing**: Understanding `select()`, `poll()`, and event loops
- **Resource Management**: Memory usage patterns and scalability trade-offs
- **Performance Analysis**: Comparing architectures under different loads
- **Dual Persistence**: RDB snapshots and AOF logging for different durability guarantees
- **Write-Ahead Logging**: AOF implementation with configurable fsync policies
- **Recovery Strategies**: AOF-first precedence with RDB fallback mechanisms
- **Background Operations**: fork(), copy-on-write, and zero-downtime maintenance
- **Process Management**: Child process coordination and cleanup (SIGCHLD)
- **File Management**: Automatic compaction, size-based triggers, and atomic operations

### Systems Programming
- **Network Programming**: TCP sockets, partial reads, connection management  
- **Protocol Implementation**: RESP parsing and response formatting
- **Signal Handling**: Graceful shutdown and child process management (SIGCHLD)
- **Buffer Management**: Production-quality data handling across network calls
- **File I/O**: Atomic operations, temporary files, and crash safety
- **Process Control**: fork(), exec(), waitpid(), and zombie prevention

### Software Architecture
- **Modular Design**: Clean separation of concerns (networking, storage, protocol)
- **Template Programming**: Generic interfaces for different storage backends
- **Code Reuse**: Shared utilities across different architectures
- **API Design**: Command-line interfaces and configuration management
- **Persistence Patterns**: Background saves, write-ahead logging, change tracking, and recovery strategies
- **Durability vs Performance**: Trade-offs between fsync policies and system performance

## Contributing

This is primarily a learning project, but contributions are welcome! Areas of interest:

### Immediate Opportunities
- **Performance Benchmarking**: Add tools to compare architecture performance
- **Additional Commands**: Implement more Redis commands (INCR, APPEND, etc.)
- **Protocol Enhancements**: Support for Redis pipelining
- **Testing**: Expand test coverage for edge cases and persistence scenarios
- **Configuration System**: Runtime configuration for persistence policies

### Advanced Features
- **Persistence Configuration**: Runtime configuration for save conditions, AOF policies, and file paths
- **Replication**: Implement basic master-slave replication
- **Clustering**: Explore Redis cluster concepts
- **Monitoring**: Add metrics and observability features
- **Memory Optimization**: Implement memory-efficient data structures

Please feel free to:
- Report bugs or unexpected behavior
- Suggest architectural improvements
- Submit pull requests with enhancements
- Share performance analysis or benchmarks

## Acknowledgments

- **Redis** for the inspiration and protocol specification
- **Stevens' Network Programming** for socket programming concepts
- **Event-driven programming patterns** from modern server architectures
- **Redis Persistence Documentation** for RDB snapshot implementation details
- **Unix Process Programming** for fork(), signals, and process management
- **C++ community** for modern C++ practices and guidelines
