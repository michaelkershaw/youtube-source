#!/bin/bash
# =============================================================================
# YouTube Source for VirtualDJ - macOS .pkg Installer Builder
# =============================================================================
# Run this script ON A MAC to build the .pkg installer.
#
# Prerequisites:
#   - macOS 11.0+ with Xcode Command Line Tools (xcode-select --install)
#   - CMake (brew install cmake)
#   - Optional: ffmpeg installed via Homebrew (brew install ffmpeg)
#     If not found, the installer will still build but ffmpeg won't be bundled.
#
# Usage:
#   cd mac
#   chmod +x build_pkg.sh
#   ./build_pkg.sh
#
# Output:
#   mac/output/YouTubeSource_3.0.0.0_macOS.pkg
# =============================================================================

set -e

# --- Auto-increment build version ---
SCRIPT_DIR_TMP="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR_TMP="$(cd "$SCRIPT_DIR_TMP/.." && pwd)"
if [ -f "$SRC_DIR_TMP/increment_version.sh" ]; then
    chmod +x "$SRC_DIR_TMP/increment_version.sh"
    "$SRC_DIR_TMP/increment_version.sh"
fi

# --- Configuration ---
# Read version from PluginVersion.h (auto-incremented above)
VERSION=$(grep '#define PLUGIN_VERSION' "$SRC_DIR_TMP/PluginVersion.h" | head -1 | sed -E 's/.*"([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)".*/\1/')
if [ -z "$VERSION" ]; then
    VERSION="3.0.0.0"
fi
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
STAGING_DIR="$SCRIPT_DIR/staging"
OUTPUT_DIR="$SCRIPT_DIR/output"
COMPONENT_PKG="$SCRIPT_DIR/YouTubeSource_Component.pkg"
FINAL_PKG="$OUTPUT_DIR/YouTubeSource_${VERSION}_macOS.pkg"

# --- Banner ---
echo ""
echo "╔══════════════════════════════════════════════╗"
echo "║  YouTube Source .pkg Installer Builder        ║"
echo "║  Version $VERSION                              ║"
echo "╚══════════════════════════════════════════════╝"
echo ""

# --- Check prerequisites ---
echo "▸ Checking prerequisites..."

if ! command -v cmake > /dev/null 2>&1; then
    echo "ERROR: CMake not found. Install with: brew install cmake"
    exit 1
fi

if ! command -v pkgbuild > /dev/null 2>&1; then
    echo "ERROR: pkgbuild not found. Install Xcode Command Line Tools: xcode-select --install"
    exit 1
fi

if ! command -v productbuild > /dev/null 2>&1; then
    echo "ERROR: productbuild not found. Install Xcode Command Line Tools: xcode-select --install"
    exit 1
fi

echo "  CMake:       $(cmake --version | head -1)"
echo "  Architecture: $(uname -m)"
echo ""

# --- Check if VirtualDJ is running ---
if pgrep -x "VirtualDJ" > /dev/null 2>&1; then
    echo "WARNING: VirtualDJ is running. The installer will ask the user to close it."
fi

# --- Step 1: Build the plugin bundle ---
echo "▸ Step 1/5: Building YouTubeSource.bundle (universal binary)..."

# Clean previous build
rm -rf "$BUILD_DIR"
cmake -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -S "$SCRIPT_DIR"

cmake --build "$BUILD_DIR" --config Release 2>&1

BUNDLE="$BUILD_DIR/YouTubeSource.bundle"
if [ ! -d "$BUNDLE" ]; then
    echo "ERROR: Build failed - YouTubeSource.bundle not found."
    exit 1
fi

echo "  Built: $BUNDLE"
echo ""

# --- Step 2: Prepare staging directory ---
echo "▸ Step 2/5: Preparing staging directory..."

rm -rf "$STAGING_DIR"
mkdir -p "$STAGING_DIR"

# Copy plugin bundle
cp -R "$BUNDLE" "$STAGING_DIR/"

# Copy UI files
mkdir -p "$STAGING_DIR/ui"
cp -R "$SRC_DIR/ui/"* "$STAGING_DIR/ui/"

# Download yt-dlp macOS binary
echo "  Downloading yt-dlp (macOS)..."
YTDLP_URL="https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp_macos"
curl -sL "$YTDLP_URL" -o "$STAGING_DIR/yt-dlp"
chmod +x "$STAGING_DIR/yt-dlp"
if [ ! -s "$STAGING_DIR/yt-dlp" ]; then
    echo "ERROR: Failed to download yt-dlp from $YTDLP_URL"
    exit 1
fi
echo "  yt-dlp downloaded: $(du -h "$STAGING_DIR/yt-dlp" | cut -f1)"

# Copy ffmpeg if available
FFMPEG_FOUND=false
if [ -f "$SRC_DIR/ffmpeg" ]; then
    cp "$SRC_DIR/ffmpeg" "$STAGING_DIR/ffmpeg"
    chmod +x "$STAGING_DIR/ffmpeg"
    FFMPEG_FOUND=true
elif command -v ffmpeg > /dev/null 2>&1; then
    cp "$(command -v ffmpeg)" "$STAGING_DIR/ffmpeg"
    chmod +x "$STAGING_DIR/ffmpeg"
    FFMPEG_FOUND=true
fi

if [ "$FFMPEG_FOUND" = true ]; then
    echo "  ffmpeg: bundled ($(du -h "$STAGING_DIR/ffmpeg" | cut -f1))"
else
    echo "  WARNING: ffmpeg not found - installer will not include it."
    echo "    Install via: brew install ffmpeg"
    echo "    Then re-run this script, or copy ffmpeg manually after install."
fi

echo "  Staging complete."
echo ""

# --- Step 3: Build component .pkg ---
echo "▸ Step 3/5: Building component package..."

# Make postinstall executable
chmod +x "$SCRIPT_DIR/scripts/postinstall"

# pkgbuild creates a component package from the staging directory.
# --install-location is a temp dir; the postinstall script copies to $HOME.
pkgbuild \
    --root "$STAGING_DIR" \
    --install-location "/tmp/yts-install" \
    --identifier "com.youtube-source.vdj.plugin" \
    --version "$VERSION" \
    --scripts "$SCRIPT_DIR/scripts" \
    "$COMPONENT_PKG"

echo "  Component package: $COMPONENT_PKG"
echo ""

# --- Step 4: Build final .pkg with productbuild ---
echo "▸ Step 4/5: Building final installer package..."

mkdir -p "$OUTPUT_DIR"

productbuild \
    --distribution "$SCRIPT_DIR/Distribution.xml" \
    --package-path "$SCRIPT_DIR" \
    --resources "$SCRIPT_DIR/resources" \
    "$FINAL_PKG"

echo "  Final package: $FINAL_PKG"
echo ""

# --- Step 5: Summary ---
echo "▸ Step 5/5: Done!"
echo ""
echo "╔══════════════════════════════════════════════╗"
echo "║  SUCCESS - Installer built!                   ║"
echo "╚══════════════════════════════════════════════╝"
echo ""
echo "  Output: $FINAL_PKG"
echo "  Size:   $(du -h "$FINAL_PKG" | cut -f1)"
echo ""
echo "  Contents:"
echo "    - YouTubeSource.bundle (universal: arm64 + x86_64)"
echo "    - UI files (index.html, app.js, styles.css)"
echo "    - yt-dlp (macOS binary)"
if [ "$FFMPEG_FOUND" = true ]; then
echo "    - ffmpeg"
else
echo "    - ffmpeg (NOT included - install separately)"
fi
echo ""
echo "  To install: Double-click the .pkg file"
echo "  To distribute: Sign and notarize with your Apple Developer ID:"
echo "    productsign --sign 'Developer ID Installer: Your Name' '$FINAL_PKG' signed.pkg"
echo "    xcrun notarytool submit signed.pkg --apple-id you@email.com --wait"
echo "    xcrun stapler staple signed.pkg"
echo ""

# Clean up intermediate component pkg
rm -f "$COMPONENT_PKG"

exit 0
