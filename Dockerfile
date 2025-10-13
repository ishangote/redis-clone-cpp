# Use Ubuntu 20.04 as base image
FROM ubuntu:20.04

# Prevent interactive prompts during installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && \
    apt-get install -y \
        cmake \
        g++ \
        make \
        git \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Build without tests
RUN mkdir -p build && \
    cd build && \
    cmake -DBUILD_TESTING=OFF .. && \
    cmake --build .

# Create data directory for persistence
RUN mkdir -p data

# Expose Redis port
EXPOSE 6379

# Run Redis clone when container starts
CMD ["./build/bin/redis-clone-cpp", "--mode=eventloop"]
