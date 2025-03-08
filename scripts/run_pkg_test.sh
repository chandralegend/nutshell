#!/bin/bash

# This script runs the package installation test

# Ensure packages directory structure is ready
mkdir -p "$HOME/.nutshell/packages"

# Build the test
cd "$(dirname "$0")/.."
make tests/test_pkg_install.test

# Run the test
./tests/test_pkg_install.test

echo "Test completed!"
