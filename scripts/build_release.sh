#!/bin/bash

# build_release.sh - Create a release bundle for Nutshell shell
# Usage: ./scripts/build_release.sh [version]

set -e  # Exit on any error

# Determine script directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
cd "$PROJECT_ROOT"

# Get version (either from argument or generate based on date)
if [ -n "$1" ]; then
    VERSION="$1"
else
    VERSION="$(date +%Y.%m.%d)"
fi

echo "Building Nutshell release version $VERSION..."

# Create build directory
BUILD_DIR="$PROJECT_ROOT/build"
DIST_DIR="$BUILD_DIR/nutshell-$VERSION"
rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR"

# Compile with optimizations
echo "Compiling with optimizations..."
make clean
CFLAGS="-O2 -DNDEBUG" make

# Create directory structure
mkdir -p "$DIST_DIR/bin"
mkdir -p "$DIST_DIR/packages"
mkdir -p "$DIST_DIR/doc"

# Copy binary and essential files
cp "$PROJECT_ROOT/nutshell" "$DIST_DIR/bin/"
cp "$PROJECT_ROOT/README.md" "$DIST_DIR/doc/"
cp -r "$PROJECT_ROOT/packages/gitify" "$DIST_DIR/packages/"

# Generate version file
echo "$VERSION" > "$DIST_DIR/VERSION"

# Create simple installer script
cat > "$DIST_DIR/install.sh" << 'EOL'
#!/bin/bash
set -e

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Default install locations
PREFIX="/usr/local"
USER_INSTALL=false

# Parse arguments
while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
        --prefix=*)
            PREFIX="${key#*=}"
            shift
            ;;
        --user)
            USER_INSTALL=true
            shift
            ;;
        *)
            echo "Unknown option: $key"
            echo "Usage: $0 [--prefix=PATH] [--user]"
            exit 1
            ;;
    esac
done

if [ "$USER_INSTALL" = true ]; then
    # User installation
    mkdir -p "$HOME/bin"
    mkdir -p "$HOME/.nutshell/packages"
    
    cp "$SCRIPT_DIR/bin/nutshell" "$HOME/bin/"
    cp -r "$SCRIPT_DIR/packages/"* "$HOME/.nutshell/packages/"
    
    echo "Nutshell installed to $HOME/bin/nutshell"
    echo "Make sure $HOME/bin is in your PATH"
    echo "You can add it with: echo 'export PATH=\$PATH:\$HOME/bin' >> ~/.bashrc"
else
    # System installation
    if [ "$(id -u)" -ne 0 ]; then
        echo "System installation requires root privileges"
        echo "Please run with sudo or use --user for user installation"
        exit 1
    fi
    
    mkdir -p "$PREFIX/bin"
    mkdir -p "$PREFIX/share/nutshell/packages"
    
    cp "$SCRIPT_DIR/bin/nutshell" "$PREFIX/bin/"
    cp -r "$SCRIPT_DIR/packages/"* "$PREFIX/share/nutshell/packages/"
    
    echo "Nutshell installed to $PREFIX/bin/nutshell"
fi

echo "Installation complete!"
EOL

# Make installer executable
chmod +x "$DIST_DIR/install.sh"

# Create archive
echo "Creating distribution archive..."
cd "$BUILD_DIR"
tar -czf "nutshell-$VERSION.tar.gz" "nutshell-$VERSION"

# Generate checksum
echo "Generating checksums..."
cd "$BUILD_DIR"
sha256sum "nutshell-$VERSION.tar.gz" > "nutshell-$VERSION.sha256"

echo "Release bundle created at $BUILD_DIR/nutshell-$VERSION.tar.gz"
echo "Checksums available at $BUILD_DIR/nutshell-$VERSION.sha256"
