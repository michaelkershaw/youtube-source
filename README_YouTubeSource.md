# YouTube Source Plugin for Virtual DJ

> **Stream and download YouTube directly inside VirtualDJ — search, preview, cache, and play with full licensing support.**

---

## What Is It?

**YouTube Source** is a professional Online Source plugin for VirtualDJ that brings the entire YouTube music library straight into your DJ software. Search by name, browse results, download as MP3 or MP4, and load tracks directly onto your decks — all without leaving VirtualDJ.

Designed for professional DJs who need access to a massive music catalogue on the fly.

---

## Key Features

### Search & Browse
- **Live YouTube Search** — search by song name, artist, or any keyword directly from the VirtualDJ browser
- **Advanced Search Window** — full-featured search dialog with results list showing title, channel, duration, and view count
- **Right-click context menu** — load to deck, add to automix sidetrack, or queue directly from search results
- **5 free searches** for unlicensed users — try before you buy

### Download & Caching
- **MP3 mode** — downloads audio-only, fast and compact
- **MP4 mode** — downloads full video+audio for video DJ setups
- **Format toggle button** — switch between MP3 and MP4 with one click
- **Smart caching** — downloaded tracks are saved locally so they load instantly next time
- **Visual download indicators** — results show blue note icon (MP3 cached) or purple play icon (MP4 cached), with green text on title/channel/duration when downloaded
- **Real-time progress** — status bar shows download progress and separate MP3/MP4 cached counts
- **12-hour cache cleanup** — old files are automatically removed to save disk space

### VirtualDJ Integration
- **Native Online Source** — appears in the VirtualDJ browser sidebar like any other source
- **Load to Deck** — double-click or right-click → Load to load a track onto any deck
- **Automix / Sidetrack** — add results directly to the automix queue
- **Format memory** — remembers your last used format (MP3 or MP4) between sessions
- **Open in Browser** — right-click any track to open the YouTube page

---

## Online Licensing System

YouTube Source uses a professional online licensing system to protect and manage access.

### For Users
- **License Manager dialog** — accessible via right-click on the YouTube Source folder → **Manage License**
- Displays: License status, License Key, Machine ID, Expiry Date + days remaining, Server URL
- **Activate** your key by entering it in the dialog and clicking Activate
- **Copy Machine ID** button — send your Machine ID to the vendor to get a key generated for your machine
- **Online key verification** — every time you open the License Manager, your key is checked against the server in real time
- Keys are **machine-bound** — a key generated for your computer cannot be used on another machine
- If a key is revoked on the server, the License Manager will immediately show **"License REVOKED"** and clear the local key
- License key format: `YT-XXXX-XXXX-XXXX-XXXX-XXXX`

### For Vendors / Admins
- **Web-based Admin Panel** at your hosting URL
- Secured with **database login** (bcrypt hashed passwords, session timeout after 2 hours)
- **Three tabs**: License Management, Admin Users, Activity Log
- Generate keys pre-bound to a specific Machine ID — only that machine can ever activate the key
- Dashboard stats: Total / Active / Unused / Expired / Revoked keys
- Per-license expiry with **days remaining** shown in green/orange/red
- Revoke, Reset, or Delete licenses at any time
- Manage multiple admin accounts with different roles (Admin / Manager)
- Full activity log: every activation, validation, and rejection with IP address and timestamp

---

## System Requirements

| Requirement | Details |
|---|---|
| VirtualDJ | Version 8 or higher |
| Windows | Windows 10 or later (32-bit or 64-bit) |
| Internet | Required for search and first download |
| Disk space | ~50MB for cache (auto-managed) |
| WebView2 Runtime | Required for modern UI (pre-installed on Windows 11 and most Windows 10 systems) |

**No additional redistributables required** — the plugin uses static linking for all C++ runtime dependencies.

If WebView2 Runtime is not installed, the plugin will automatically fall back to a legacy Win32 dialog interface.

All required tools (yt-dlp, ffmpeg) are bundled with the plugin installer — no separate downloads needed.

---

## Installation

1. Run the provided installer or copy the DLL to:
   ```
   C:\Users\[YourName]\AppData\Local\VirtualDJ\Plugins64\OnlineSources\
   ```
2. Restart VirtualDJ
3. YouTube Source will appear in the browser sidebar
4. Right-click the folder → **Manage License** to log in and activate

---

## How to Activate Your License

1. Purchase a license from our website
2. Open VirtualDJ and navigate to the YouTube Source folder
3. Right-click → **Manage License** (or open the plugin and click the License tab)
4. Enter your email and password to log in to your account
5. Your license will be automatically activated for your machine

The plugin remembers your login state, so you won't need to log in again.

---

## Troubleshooting

| Issue | Fix |
|---|---|
| "Login Failed" | Check your internet connection; verify your email and password |
| "License REVOKED" | Contact support — your license has been disabled |
| Search returns no results | Check internet connection; yt-dlp may need updating |
| Download stuck | Check available disk space; try switching format (MP3/MP4) |
| Plugin not showing in VirtualDJ | Ensure DLL is in `OnlineSources` folder, not `Plugins64` root; restart VirtualDJ |
| Cache files not loading | Files expire after 12 hours; search and download again |

---

## Privacy & Security

- License keys are validated securely over HTTPS
- Only your Machine ID and License Key are sent to the server — no personal data
- Admin panel uses bcrypt password hashing and session-based authentication
- All license activity is logged server-side for audit purposes

---

## Version History

### v3.0 — Current
- Brand-new WebView2 interface: thumbnails, result cards, tabs (Search / Playlists / Trending / Charts / History / License)
- YouTube playlist import with "Add all to Automix" and "Download all" batch actions
- Trending and genre Charts browsing, plus persistent search history
- New VirtualDJ sidebar folders: Trending, Charts Top 100, Cached Tracks
- License Manager rebuilt inside the new UI (activate key, marketplace login, live status)
- Engine rewrite: piped yt-dlp processes (no more temp .bat files), JSON parsing, retry with backoff
- Persistent cache index with automatic reconciliation
- yt-dlp automatic self-update (once per 72 hours)
- Legacy Win32 dialog kept as automatic fallback when WebView2 runtime is missing

### v2.0
- Online licensing system with machine-ID binding
- License Manager UI dialog with server validation
- Admin web panel with login, tabs, admin user management
- MP3 and MP4 download format support
- Advanced Search dialog with right-click context menu
- Visual download indicators (blue MP3 / purple MP4 / green downloaded text)
- Days remaining on license expiry
- 12-hour cache auto-cleanup
- Smart cached track count in status bar

### v1.0 — Initial Release
- Basic YouTube audio extraction via yt-dlp
- Simple search via VirtualDJ search bar
- MP3 caching and local playback
- Free search limit (5 searches)

---

## Support

For licensing, key generation, or technical support, contact us via our Facebook page or support channel.

> Please respect YouTube's Terms of Service when using this plugin.
