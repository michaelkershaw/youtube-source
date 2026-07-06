# YouTube Source for VirtualDJ — macOS Port

## Quick Start: Build the .pkg Installer

```bash
# 1. Install prerequisites (one-time)
xcode-select --install
brew install cmake

# 2. Optional: install ffmpeg (will be bundled in the .pkg)
brew install ffmpeg

# 3. Build the .pkg installer
cd mac
chmod +x build_pkg.sh
./build_pkg.sh

# 4. Install: double-click the .pkg
open output/YouTubeSource_3.0.0.0_macOS.pkg
```

The `.pkg` installer handles everything: plugin bundle, UI, yt-dlp, and ffmpeg.

## What's Ported

- **Plugin core** — `YouTubeSource.cpp` compiles on macOS (`#ifdef` platform splits)
- **yt-dlp engine** — `YtDlpRunner.cpp` uses fork/exec + pipes on macOS
- **UI** — same HTML/JS/CSS (`ui/` folder), hosted in a native WKWebView window (`WebViewHostMac.mm`)
- **Machine ID** — uses the Mac hardware UUID (`IOPlatformUUID`) + hostname + username
- **License HTTP** — uses the system `curl` binary
- **Data location** — `~/Documents/VirtualDJ/youtube-cloud/` (history, cache, license, settings)

## Not Available on macOS

- Legacy Win32 fallback dialog (not needed — WKWebView always available)
- In-place plugin self-update (updates ship via installer instead)

## .pkg Installer Details

The installer (`build_pkg.sh`) performs these steps:

1. **Build** the universal `YouTubeSource.bundle` (arm64 + x86_64) via CMake
2. **Stage** the bundle + UI + yt-dlp + ffmpeg into a staging directory
3. **pkgbuild** creates a component package with a `postinstall` script
4. **productbuild** wraps it with a polished installer UI (welcome, license, conclusion)

The `postinstall` script detects the Mac's architecture and copies files to:
- **Apple Silicon:** `~/Library/Application Support/VirtualDJ/PluginsMacArm/OnlineSources/`
- **Intel:** `~/Library/Application Support/VirtualDJ/Plugins64/OnlineSources/`

### Installer Files

| File | Purpose |
|------|---------|
| `build_pkg.sh` | Main build script — runs CMake, pkgbuild, productbuild |
| `Distribution.xml` | Installer UI metadata (title, version, screens) |
| `scripts/postinstall` | Copies staged payload to the correct VirtualDJ directory |
| `resources/welcome.html` | Welcome screen shown by the installer |
| `resources/conclusion.html` | Post-install instructions screen |
| `resources/license.rtf` | License agreement shown by the installer |

## Manual Build (without .pkg)

If you prefer to build and install manually:

```bash
# Build
cmake -B build -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Install
chmod +x install_mac.sh
./install_mac.sh
```

## Folder Layout After Install

```
~/Library/Application Support/VirtualDJ/PluginsMacArm/OnlineSources/   (Apple Silicon)
├── YouTubeSource.bundle
└── youtube-source/
    ├── ui/            (index.html, app.js, styles.css)
    ├── yt-dlp         (macOS binary)
    └── ffmpeg         (macOS binary)
```

Intel Macs use `Plugins64` instead of `PluginsMacArm`.

## Distribution (Signing & Notarization)

For public distribution, sign and notarize the .pkg:

```bash
# Sign with your Apple Developer ID
productsign --sign "Developer ID Installer: Your Name" \
    output/YouTubeSource_3.0.0.0_macOS.pkg \
    output/YouTubeSource_3.0.0.0_macOS_signed.pkg

# Notarize
xcrun notarytool submit output/YouTubeSource_3.0.0.0_macOS_signed.pkg \
    --apple-id you@email.com \
    --team-id ABCDE12345 \
    --password app-specific-password \
    --wait

# Staple the ticket
xcrun stapler staple output/YouTubeSource_3.0.0.0_macOS_signed.pkg
```

## Notes for Testing

- First run: macOS Gatekeeper may block yt-dlp/ffmpeg. The `postinstall` script
  removes the quarantine attribute automatically. If issues persist, run:
  `xattr -d com.apple.quarantine <file>` or right-click > Open once.
- License machine IDs will differ from the same user's Windows machine
  (expected: each device counts as one activation).
- Log file: `~/Documents/VirtualDJ/youtube-cloud/YouTubeSource.log`
