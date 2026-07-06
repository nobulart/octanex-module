#!/bin/bash
set -e

# Launch Octane X with module logging enabled
# This is helpful for debugging module loading

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# Octane X application
OCTANE_APP="/Applications/Octane X.app/Contents/MacOS/Octane X"

if [ ! -f "$OCTANE_APP" ]; then
    echo "Error: Octane X not found at $OCTANE_APP"
    exit 1
fi

# Check if module is installed
MODULE_PATH="/Applications/Octane X.app/Contents/Resources/modules/octanex.dylib"
if [ -f "$MODULE_PATH" ]; then
    echo "✓ Module found: $MODULE_PATH"
else
    echo "ℹ Module not yet installed. Run ./scripts/install-to-octane.sh"
    echo "  (The module will be used if present at runtime)"
fi

echo "=== Starting Octane X with module logging ==="
echo ""

# Start Octane X with module log flag
# --module-log enables detailed module loading messages
"$OCTANE_APP" --module-log
