#!/bin/bash
set -e

# Build the Octanex C++ module
# Produces modules/octanex.d.dylib compatible with Octane X

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"
MODULE_DIR="$PROJECT_DIR/modules"

echo "=== Building OctaneX Bridge Module ==="
echo "Project: $PROJECT_DIR"
echo "Build:   $BUILD_DIR"
echo "Module:  $MODULE_DIR"

# Clean previous build
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
mkdir -p "$MODULE_DIR"

cd "$BUILD_DIR"

# Run cmake
cmake .. \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DCMAKE_BUILD_TYPE=Release \
    -Wno-dev

# Build
make -j$(sysctl -n hw.ncpu 2>/dev/null || gnumactl -n 2>/dev/null || echo 4)

# Verify the dylib
echo ""
echo "=== Verification ==="
if [ -f "$MODULE_DIR/octanex.dylib" ]; then
    echo "✓ octanex.dylib created"
    echo ""
    echo "=== dylib info ==="
    otool -L "$MODULE_DIR/octanex.dylib"
    echo ""
    file "$MODULE_DIR/octanex.dylib"
else
    echo "✗ octanex.dylib NOT found at $MODULE_DIR"
    exit 1
fi

# Copy to Octane's modules directory
OCTANE_MODULES="/Applications/Octane X.app/Contents/Resources/modules"
if [ -d "$OCTANE_MODULES" ]; then
    echo ""
    echo "=== Copying to Octane X ==="
    cp "$MODULE_DIR/octanex.dylib" "$OCTANE_MODULES/"
    echo "✓ Copied to $OCTANE_MODULES"
else
    echo ""
    echo "ℹ Octane X modules dir: $OCTANE_MODULES"
    echo "  (not present, module must be installed manually)"
fi

echo ""
echo "=== Build complete ==="
