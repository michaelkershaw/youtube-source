#!/bin/bash
# YouTube Source for VirtualDJ - macOS installer script
# Run on a Mac after building with CMake (see CMakeLists.txt)

set -e

echo "========================================"
echo " YouTube Source Plugin - macOS Install"
echo "========================================"

# VirtualDJ plugin folders (PluginsMacArm = Apple Silicon native, Plugins64 = Intel)
VDJ="$HOME/Library/Application Support/VirtualDJ"
if [[ "$(uname -m)" == "arm64" ]]; then
    PLUGDIR="$VDJ/PluginsMacArm/OnlineSources"
else
    PLUGDIR="$VDJ/Plugins64/OnlineSources"
fi

BUNDLE="build/YouTubeSource.bundle"
if [ ! -d "$BUNDLE" ]; then
    echo "ERROR: $BUNDLE not found. Build first:"
    echo "  cmake -B build -DCMAKE_OSX_ARCHITECTURES=\"arm64;x86_64\" -DCMAKE_BUILD_TYPE=Release"
    echo "  cmake --build build"
    exit 1
fi

if pgrep -x "VirtualDJ" > /dev/null; then
    echo "ERROR: VirtualDJ is running. Please close it first."
    exit 1
fi

mkdir -p "$PLUGDIR/youtube-source/ui"

echo "Installing plugin bundle..."
rm -rf "$PLUGDIR/YouTubeSource.bundle"
cp -R "$BUNDLE" "$PLUGDIR/"

echo "Installing UI..."
cp -R ../ui/* "$PLUGDIR/youtube-source/ui/"

echo "Downloading yt-dlp (macOS build)..."
curl -sL "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp_macos" \
     -o "$PLUGDIR/youtube-source/yt-dlp"
chmod +x "$PLUGDIR/youtube-source/yt-dlp"

echo ""
echo "NOTE: ffmpeg must also be present. Install via:"
echo "  brew install ffmpeg && cp \"\$(which ffmpeg)\" \"$PLUGDIR/youtube-source/ffmpeg\""
echo ""

if command -v ffmpeg > /dev/null; then
    cp "$(which ffmpeg)" "$PLUGDIR/youtube-source/ffmpeg"
    echo "ffmpeg copied from Homebrew installation."
fi

# Remove quarantine and ad-hoc sign so Gatekeeper doesn't block the plugin
xattr -dr com.apple.quarantine "$PLUGDIR" 2>/dev/null || true
codesign --force --deep -s - "$PLUGDIR/YouTubeSource.bundle" 2>/dev/null || true

echo "========================================"
echo " SUCCESS! Installed to:"
echo " $PLUGDIR"
echo "========================================"
echo "Start VirtualDJ - YouTube Source appears in the sidebar."
