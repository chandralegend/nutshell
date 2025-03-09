#!/bin/bash

# Run AI integration tests
echo "Setting up AI test environment..."

# Create necessary directories
mkdir -p "$HOME/.nutshell"

# Remove any existing API key file to start with clean state
rm -f "$HOME/.nutshell/openai_key"

# Build all AI-related tests
cd "$(dirname "$0")/.."
echo "Building AI tests..."
make tests/test_ai_integration.test
make tests/test_openai_commands.test
make tests/test_ai_shell_integration.test

# Run the tests with debugging enabled
echo "Running AI tests with debugging enabled..."
export NUT_DEBUG=1
export NUT_DEBUG_AI=1
export NUT_DEBUG_AI_SHELL=1
export NUT_DEBUG_AI_VERBOSE=1  # Enable verbose API response logging
export NUTSHELL_TESTING=1      # Set testing mode to use mocks instead of real API

# Run each test separately to isolate any failures
echo "Running AI integration test..."
./tests/test_ai_integration.test

echo "Running OpenAI commands test..."
./tests/test_openai_commands.test

echo "Running AI shell integration test..."
./tests/test_ai_shell_integration.test

echo "All AI tests completed!"
