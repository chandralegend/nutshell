#!/bin/bash

# Debug script specifically for AI components
# This script runs nutshell with all AI debugging enabled

echo "Starting Nutshell with AI debugging enabled..."

# Make sure the nutshell binary exists
if [ ! -f "./nutshell" ]; then
    echo "Building Nutshell..."
    make clean && make
fi

# Set debugging environment variables
export NUT_DEBUG=1              # General debug
export NUT_DEBUG_AI=1           # AI module debug
export NUT_DEBUG_AI_SHELL=1     # AI shell integration debug
export NUT_DEBUG_AI_VERBOSE=0   # Set to 1 to see full API responses

echo "Debug flags set:"
echo "NUT_DEBUG=$NUT_DEBUG"
echo "NUT_DEBUG_AI=$NUT_DEBUG_AI"
echo "NUT_DEBUG_AI_SHELL=$NUT_DEBUG_AI_SHELL"
echo "NUT_DEBUG_AI_VERBOSE=$NUT_DEBUG_AI_VERBOSE"

echo "Starting Nutshell with AI debugging..."
echo "--------------------------------------"
./nutshell

# If you have a test OpenAI API key, you can uncomment this line:
# OPENAI_API_KEY=your_test_key_here ./nutshell
