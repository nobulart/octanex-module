#!/bin/bash
set -e

# Install the Octanex module to Octane X's modules directory
# Usage: ./scripts/install-to-octane.sh [module_path]

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
MODULE_DIR="$PROJECT_DIR/modules"
MODULE_NAME="octanex.dylib"

# Determine module path
if [ -n "$1" ]; then
    MODULE_PATH="$1"
else
    MODULE_PATH="$MODULE_DIR/$MODULE_NAME"
fi

if [ ! -f "$MODULE_PATH" ]; then
    echo "Error: Module not found at $MODULE_PATH"
    echo "Run ./scripts/build-release.sh first."
    exit 1
fi

echo "=== Installing OctaneX Module ==="
echo "Source: $MODULE_PATH"

# Octane X modules directory (container path on macOS)
# Primary: container sandbox (what a running Octane X reads from)
# Secondary: app bundle (for system-wide installs)
OCTANE_MODULES="/Users/craig/Library/Containers/com.otoy.rndrviewer/Data/Library/OctaneRender/modules"
OCTANE_MODULES_ALT="/Applications/Octane X.app/Contents/Resources/modules"

# Create the directory if it doesn't exist
mkdir -p "$OCTANE_MODULES"

# Install the module
cp "$MODULE_PATH" "$OCTANE_MODULES/"
echo "✓ Installed to $OCTANE_MODULES"

echo ""
echo "=== Verification ==="
echo "Module file: $(ls -la "$OCTANE_MODULES/$MODULE_NAME")"
echo "Module size: $(wc -c < "$OCTANE_MODULES/$MODULE_NAME") bytes"

echo ""
echo "=== How to use ==="
echo "1. Launch Octane X"
echo "   /Applications/Octane X.app/Contents/MacOS/Octane X"
echo ""
echo "2. Check module logs"
echo "   /Applications/Octane X.app/Contents/MacOS/Octane X --module-log"
echo ""
echo "3. Look for: 'Octanex Module loaded (Command module, type COMMAND)'"
