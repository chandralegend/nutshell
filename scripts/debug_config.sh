#!/bin/bash

# Set debug environment variables
export NUT_DEBUG=1
export NUT_DEBUG_CONFIG=1

# Run the configuration test with debugging
make test-config

# Or run the shell with debugging
# make && ./nutshell
