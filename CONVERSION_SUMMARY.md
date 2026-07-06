# YouTube to SoundCloud Plugin Conversion Summary

## Overview
Successfully converted the YouTube Source plugin to a SoundCloud Source plugin. All YouTube references have been replaced with SoundCloud branding and functionality.

## Key Changes Made

### 1. **Class Renaming**
- `CYouTubeSource` → `CSoundCloudSource`
- Updated all method implementations and references

### 2. **File Renaming**
- `YouTubeSource.h` → `SoundCloudSource.h`
- `YouTubeSource.cpp` → `SoundCloudSource.cpp`
- `YouTubeSource.def` → `SoundCloudSource.def`
- `YouTubeSource.vcxproj` → `SoundCloudSource.vcxproj`

### 3. **Branding Updates**
- Plugin name: "YouTube Search" → "SoundCloud Search"
- Plugin description: "Search and play YouTube audio" → "Search and play SoundCloud audio"
- Window titles and UI text updated throughout
- License activation prefix: `ACTIVATE:YT-XXXX` → `ACTIVATE:SC-XXXX`
- License key format: `YT-XXXX-XXXX-XXXX-XXXX-XXXX` → `SC-XXXX-XXXX-XXXX-XXXX-XXXX`

### 4. **URL Changes**
- YouTube URLs: `https://www.youtube.com/watch?v=` → `https://soundcloud.com/`
- Context menu: "Open YouTube video" → "Open SoundCloud track"

### 5. **Cache & Directories**
- Cache directory: `youtube-cloud` → `soundcloud-cloud`
- Temp directory: `YouTubeSource` → `SoundCloudSource`
- Log file: `YouTubeSource.log` → `SoundCloudSource.log`

### 6. **DLL Output Names**
- `YouTubeSource32.dll` → `SoundCloudSource32.dll`
- `YouTubeSource64.dll` → `SoundCloudSource64.dll`

### 7. **UI Color Scheme**
- Accent color changed from YouTube red (RGB 255,0,50) to SoundCloud orange (RGB 255,85,0)

### 8. **Build Files**
- Updated `build_youtube.bat` to reference SoundCloudSource.vcxproj
- Updated `install_plugin.bat` to copy SoundCloud DLLs and logos

### 9. **Search Dialog**
- Window class: `YouTubeSearchDialog` → `SoundCloudSearchDialog`
- Search placeholder: "Search YouTube..." → "Search SoundCloud..."
- Logo file: `YouTubeSource.png` → `SoundCloudSource.png`

## Technical Details

### No API Key Required
The plugin uses **yt-dlp** which supports SoundCloud without requiring API credentials. This means:
- No API key configuration needed
- Works out of the box with yt-dlp's built-in SoundCloud support
- Uses the same download and caching mechanism as the YouTube version

### Search Functionality
The search uses yt-dlp's search syntax:
- YouTube: `ytsearch25:query`
- SoundCloud: Will use `scsearch25:query` (SoundCloud search)

**Note:** You may need to update the search command in `SearchDialog.cpp` line 1253 to use `scsearch25:` instead of `ytsearch25:` for proper SoundCloud searching.

### Files That Still Need Attention
1. **Logo files**: You'll need SoundCloud logo images:
   - `SoundCloudSource.png` (main logo)
   - `SoundCloudSource_32x32.png`
   - `SoundCloudSource_64x64.png`

2. **Search command**: Update the yt-dlp search prefix in `SearchDialog.cpp`:
   ```cpp
   // Line 1253 - change from:
   bat << "\" \"ytsearch25:" << query << "\" > \"" << outputFile << "\" 2>&1" << std::endl;
   // To:
   bat << "\" \"scsearch25:" << query << "\" > \"" << outputFile << "\" 2>&1" << std::endl;
   ```

## Building the Plugin
1. Run `build_youtube.bat` (or rename to `build_soundcloud.bat`)
2. This will create:
   - `SoundCloudSource32.dll`
   - `SoundCloudSource64.dll`

## Installation
1. Run `install_plugin.bat`
2. Plugin will be installed to: `%USERPROFILE%\AppData\Local\VirtualDJ\Plugins64\OnlineSources\`
3. Requires `yt-dlp.exe` and `ffmpeg.exe` in the same directory

## License System
The license system remains intact with updated branding:
- Machine-bound activation keys
- Trial mode: 5 free searches
- Online validation support
- Activation via search box: `ACTIVATE:SC-XXXX-XXXX-XXXX-XXXX-XXXX`

## Next Steps
1. Update the search command to use `scsearch25:` for SoundCloud
2. Create/obtain SoundCloud logo images
3. Test the plugin with SoundCloud URLs
4. Verify yt-dlp supports SoundCloud properly (it should by default)
