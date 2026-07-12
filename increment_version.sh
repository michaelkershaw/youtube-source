#!/bin/bash
# =============================================================================
# Auto-increment build version in PluginVersion.h
# Mac equivalent of increment_version.ps1
#
# Reads PLUGIN_VERSION (e.g. "3.0.0.11"), increments the 4th number (build),
# computes PLUGIN_VERSION_CODE = major*10000 + minor*1000 + patch*100 + build,
# and writes the updated PluginVersion.h.
# =============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
VERSION_FILE="$SCRIPT_DIR/PluginVersion.h"

if [ ! -f "$VERSION_FILE" ]; then
    echo "ERROR: PluginVersion.h not found at $VERSION_FILE"
    exit 1
fi

# Extract current version string (e.g. "3.0.0.11")
CURRENT=$(grep '#define PLUGIN_VERSION' "$VERSION_FILE" | head -1 | sed -E 's/.*"([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)".*/\1/')

if [ -z "$CURRENT" ]; then
    echo "ERROR: Could not parse PLUGIN_VERSION from $VERSION_FILE"
    exit 1
fi

# Parse version components
MAJOR=$(echo "$CURRENT" | cut -d. -f1)
MINOR=$(echo "$CURRENT" | cut -d. -f2)
PATCH=$(echo "$CURRENT" | cut -d. -f3)
BUILD=$(echo "$CURRENT" | cut -d. -f4)

# Increment build number
BUILD=$((BUILD + 1))

# Compute version code
VERSION_CODE=$((MAJOR * 10000 + MINOR * 1000 + PATCH * 100 + BUILD))

NEW_VERSION="${MAJOR}.${MINOR}.${PATCH}.${BUILD}"

echo "Version: $CURRENT -> $NEW_VERSION (code: $VERSION_CODE)"

# Write updated PluginVersion.h
cat > "$VERSION_FILE" << EOF
#pragma once

#define PLUGIN_VERSION      "${NEW_VERSION}"
#define PLUGIN_VERSION_CODE  ${VERSION_CODE}
EOF

echo "Updated $VERSION_FILE"
