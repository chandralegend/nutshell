#!/bin/bash

# This script runs the theme system tests
echo "Setting up theme test environment..."

# Ensure themes directory structure is ready
mkdir -p "$HOME/.nutshell/themes"
echo "Created directory: $HOME/.nutshell/themes"

# Show current directory for debugging
echo "Current directory: $(pwd)"
echo "Contents of themes directory:"
ls -la ./themes/

# Copy test themes to user directory 
echo "Copying theme files to user directory..."
cp -fv "$PWD/themes/"*.json "$HOME/.nutshell/themes/"

# Check that themes were copied
echo "Contents of user themes directory:"
ls -la "$HOME/.nutshell/themes/"

# Build the test
cd "$(dirname "$0")/.."
echo "Building theme test..."
make tests/test_theme.test

# Run the test with some debug environment variables
echo "Running theme test with debugging..."
# Set both general debug and theme-specific debug
NUT_DEBUG=1 NUT_DEBUG_THEME=1 ASAN_OPTIONS=detect_leaks=0 ./tests/test_theme.test

echo "Theme tests completed!"
