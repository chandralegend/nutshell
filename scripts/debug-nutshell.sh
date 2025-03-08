#!/bin/bash

# Run nutshell with environment variables for debugging
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Build nutshell if needed
echo "Building nutshell..."
cd "$PROJECT_ROOT"
make clean && make

echo "Starting nutshell in debug mode..."
export NUT_DEBUG=1

# Get the proper Jansson library path
JANSSON_LIB_PATH=$(pkg-config --libs jansson 2>/dev/null || echo "-L/usr/local/opt/jansson/lib -ljansson")
JANSSON_INCLUDE_PATH=$(pkg-config --cflags jansson 2>/dev/null || echo "-I/usr/local/opt/jansson/include")

# Enable core dumps
ulimit -c unlimited

# First try a basic build without sanitizer
echo "Building debug version without sanitizer..."
clang -g -I./include -I/usr/local/include -I/opt/homebrew/include $JANSSON_INCLUDE_PATH \
    -o nutshell-debug $PROJECT_ROOT/src/core/*.c $PROJECT_ROOT/src/pkg/*.c $PROJECT_ROOT/src/utils/*.c $PROJECT_ROOT/src/ai/*.c \
    -lreadline -lcurl -ldl -lssl -lcrypto $JANSSON_LIB_PATH

if [ -f "./nutshell-debug" ]; then
    echo "Running debug build..."
    ./nutshell-debug
    EXIT_CODE=$?
    if [ $EXIT_CODE -ne 0 ]; then
        echo "Debug build crashed with exit code $EXIT_CODE"
        
        # If LLDB is available, try to run with it for more details
        if command -v lldb >/dev/null 2>&1; then
            echo "Running with LLDB for more details..."
            lldb -- ./nutshell-debug
        elif command -v gdb >/dev/null 2>&1; then
            echo "Running with GDB for more details..."
            gdb --args ./nutshell-debug
        else
            echo "Consider installing LLDB or GDB for better debugging"
            echo "Falling back to regular binary"
            ./nutshell
        fi
    fi
else
    echo "Failed to build debug version, falling back to regular binary"
    ./nutshell
fi

echo "Debug session ended"
