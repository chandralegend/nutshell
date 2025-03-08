#!/bin/bash

# Ensure scripts are executable
chmod +x packages/gitify/gitify.sh
chmod +x scripts/generate_checksum.sh

# Generate checksum for gitify package
./scripts/generate_checksum.sh gitify

# Create package installation directory in user's home if using user install
if [ ! -d "$HOME/.nutshell/packages" ]; then
    mkdir -p "$HOME/.nutshell/packages"
fi

# Copy gitify package to user's installation
cp -r packages/gitify "$HOME/.nutshell/packages/"

echo "Gitify package installed successfully!"
echo "You can use it by typing 'gitify' in the nutshell"
echo "Make sure to rebuild nutshell with 'make' to include the new command registration"
