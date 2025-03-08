#!/bin/bash

# Install a theme from a JSON file or URL
# Usage: ./install_theme.sh <path_or_url>

set -e

# Ensure themes directory exists
THEME_DIR="$HOME/.nutshell/themes"
mkdir -p "$THEME_DIR"

if [ $# -lt 1 ]; then
    echo "Usage: $0 <theme_file_or_url>"
    exit 1
fi

SOURCE="$1"

# Check if it's a URL (starts with http:// or https://)
if [[ "$SOURCE" == http://* || "$SOURCE" == https://* ]]; then
    # Download the theme
    echo "Downloading theme from $SOURCE"
    TEMP_FILE=$(mktemp)
    if command -v curl &> /dev/null; then
        curl -s "$SOURCE" -o "$TEMP_FILE"
    elif command -v wget &> /dev/null; then
        wget -q -O "$TEMP_FILE" "$SOURCE"
    else
        echo "Error: Neither curl nor wget is installed"
        exit 1
    fi
    
    # Extract the filename from URL
    FILENAME=$(basename "$SOURCE")
    if [[ ! "$FILENAME" == *.json ]]; then
        FILENAME="downloaded_theme.json"
    fi
    
    # Check if it's valid JSON
    if ! jq . "$TEMP_FILE" > /dev/null 2>&1; then
        echo "Error: Not a valid JSON file"
        rm "$TEMP_FILE"
        exit 1
    fi
    
    # Get theme name from JSON if possible
    if command -v jq &> /dev/null; then
        THEME_NAME=$(jq -r '.name' "$TEMP_FILE" 2>/dev/null)
        if [ "$THEME_NAME" != "null" ] && [ -n "$THEME_NAME" ]; then
            FILENAME="${THEME_NAME}.json"
        fi
    fi
    
    # Copy to themes directory
    cp "$TEMP_FILE" "$THEME_DIR/$FILENAME"
    rm "$TEMP_FILE"
    
    echo "Theme installed to $THEME_DIR/$FILENAME"
    echo "You can now apply it with: theme ${FILENAME%.json}"
    
else
    # Assume it's a local file
    if [ ! -f "$SOURCE" ]; then
        echo "Error: File not found: $SOURCE"
        exit 1
    fi
    
    # Check if it's valid JSON
    if ! jq . "$SOURCE" > /dev/null 2>&1; then
        echo "Error: Not a valid JSON file"
        exit 1
    fi
    
    # Get the filename
    FILENAME=$(basename "$SOURCE")
    
    # Get theme name from JSON if possible
    if command -v jq &> /dev/null; then
        THEME_NAME=$(jq -r '.name' "$SOURCE" 2>/dev/null)
        if [ "$THEME_NAME" != "null" ] && [ -n "$THEME_NAME" ]; then
            FILENAME="${THEME_NAME}.json"
        fi
    fi
    
    # Copy to themes directory
    cp "$SOURCE" "$THEME_DIR/$FILENAME"
    
    echo "Theme installed to $THEME_DIR/$FILENAME"
    echo "You can now apply it with: theme ${FILENAME%.json}"
fi
