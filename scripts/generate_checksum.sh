#!/bin/bash

# Generate SHA256 checksum for a package
# Usage: ./generate_checksum.sh package_name

if [ -z "$1" ]; then
  echo "Usage: $0 package_name"
  exit 1
fi

PACKAGE_NAME="$1"
PACKAGES_DIR="/Users/chandralegend/Desktop/nutshell/packages"
PACKAGE_DIR="$PACKAGES_DIR/$PACKAGE_NAME"

if [ ! -d "$PACKAGE_DIR" ]; then
  echo "Package $PACKAGE_NAME not found in $PACKAGES_DIR"
  exit 1
fi

# Create tarball of the package
TEMP_DIR=$(mktemp -d)
TAR_FILE="$TEMP_DIR/$PACKAGE_NAME.tar.gz"

# Exclude manifest.json since it contains the checksum we'll update
tar -czf "$TAR_FILE" -C "$PACKAGE_DIR" --exclude="manifest.json" .

# Generate SHA256 checksum
CHECKSUM=$(openssl dgst -sha256 "$TAR_FILE" | awk '{print $2}')

# Update manifest.json with the new checksum
MANIFEST_FILE="$PACKAGE_DIR/manifest.json"
TMP_MANIFEST=$(mktemp)

# Replace the checksum in the manifest file
sed "s/\"sha256\":.*/\"sha256\": \"$CHECKSUM\",/" "$MANIFEST_FILE" > "$TMP_MANIFEST"
mv "$TMP_MANIFEST" "$MANIFEST_FILE"

echo "Updated checksum for $PACKAGE_NAME: $CHECKSUM"

# Cleanup
rm -rf "$TEMP_DIR"
