# License Manager Implementation Guide

## Complete Documentation of TellyMedia Plugin Changes

This guide covers all changes made to implement the enhanced license manager with sub-license support, 14-day token expiration, and improved UI layout.

---

## Table of Contents
1. [Plugin Header Changes](#1-plugin-header-changes)
2. [Plugin Implementation Changes](#2-plugin-implementation-changes)
3. [UI Changes](#3-ui-changes)
4. [Server-Side Changes](#4-server-side-changes)
5. [Step-by-Step Implementation Guide](#5-step-by-step-implementation-guide)

---

## 1. Plugin Header Changes

### File: `TellyMediaPlugin.h` (or your plugin's header file)

Add these member variables to your plugin class:

```cpp
// ─── Licensing (public for UI access) ───────────────────────────────────
enum LicenseStatus { LIC_UNCHECKED, LIC_VALID, LIC_EXPIRED, LIC_INVALID, LIC_REVOKED, LIC_ERROR };
LicenseStatus m_licStatus;
wchar_t m_szUserEmail[256];       // User's email address
char    m_szLicenseKey[64];       // License key from server
char    m_szAuthToken[512];       // JWT or session token (cached in memory only)
bool    m_bSavePassword;          // Save password preference
char    m_szLicExpiry[24];        // Expiry date string from server
char    m_szTokenExpiry[24];      // Token expiration date (14 days from login)
char    m_szLicenseType[32];      // monthly/yearly/lifetime
char    m_szLicMessage[128];      // Last server message
int     m_nPluginId;              // Plugin ID from marketplace
int     m_nMaxActivations;        // Device activation limit
int     m_nCurrentActivations;    // Current device count
bool    m_bLicensed;              // Quick flag: is plugin licensed?
bool    m_bIsSharedLicense;       // Is this a shared/sub-license?
char    m_szSharedOwnerName[128]; // Name of license owner (if shared)
char    m_szSharedOwnerEmail[128];// Email of license owner (if shared)
char    m_szSharedDate[24];       // Date license was shared
int     m_nMaxSubLicenses;        // Maximum sub-licenses allowed
int     m_nCurrentSubLicenses;    // Current active sub-licenses
```

---

## 2. Plugin Implementation Changes

### File: `TellyMediaPlugin.cpp` (or your plugin's implementation file)

#### A. Initialize Fields in Constructor

```cpp
// Initialize account-based licensing fields
m_szUserEmail[0] = L'\0';
m_szLicenseKey[0] = '\0';
m_szAuthToken[0] = '\0';
m_szLicExpiry[0] = '\0';
m_szTokenExpiry[0] = '\0';
m_szLicenseType[0] = '\0';
m_szLicMessage[0] = '\0';
m_nPluginId = 0;
m_nMaxActivations = 0;
m_nCurrentActivations = 0;
m_nMaxSubLicenses = 0;
m_nCurrentSubLicenses = 0;
m_licStatus = LIC_UNCHECKED;
m_bLicensed = false;
m_bIsSharedLicense = false;
m_szSharedOwnerName[0] = '\0';
m_szSharedOwnerEmail[0] = '\0';
m_szSharedDate[0] = '\0';
m_bSavePassword = false;
```

#### B. Update LicenseLogin Function

Parse all fields from server response:

```cpp
void YourPlugin::LicenseLogin(const wchar_t* email, const wchar_t* password)
{
    // ... existing code to send request ...
    
    // Parse response
    char status[32] = {}, message[128] = {}, licenseKey[64] = {}, authToken[512] = {};
    char expiry[32] = {}, tokenExpiry[32] = {}, licenseType[32] = {};
    int pluginId = 0, maxActivations = 0, currentActivations = 0;
    
    JsonGetString(response, "status", status, sizeof(status));
    JsonGetString(response, "message", message, sizeof(message));
    JsonGetString(response, "license_key", licenseKey, sizeof(licenseKey));
    JsonGetString(response, "auth_token", authToken, sizeof(authToken));
    JsonGetString(response, "expires", expiry, sizeof(expiry));
    JsonGetString(response, "token_expires", tokenExpiry, sizeof(tokenExpiry));
    JsonGetString(response, "license_type", licenseType, sizeof(licenseType));
    JsonGetInt(response, "plugin_id", &pluginId);
    JsonGetInt(response, "max_activations", &maxActivations);
    JsonGetInt(response, "current_activations", &currentActivations);
    
    // Parse sub-license counts
    int maxSubLicenses = 0, currentSubLicenses = 0;
    JsonGetInt(response, "max_sub_licenses", &maxSubLicenses);
    JsonGetInt(response, "current_sub_licenses", &currentSubLicenses);
    
    // Parse shared license info
    char isSharedStr[16] = {};
    JsonGetString(response, "is_shared", isSharedStr, sizeof(isSharedStr));
    bool isShared = (strcmp(isSharedStr, "true") == 0 || strcmp(isSharedStr, "1") == 0 || 
                     strstr(response, "\"is_shared\":true") != nullptr);
    
    char sharedOwnerName[128] = {}, sharedOwnerEmail[128] = {}, sharedDate[24] = {};
    if (isShared) {
        // Parse nested shared_info object
        const char* sharedInfoStart = strstr(response, "\"shared_info\":");
        if (sharedInfoStart) {
            JsonGetString(sharedInfoStart, "owner_name", sharedOwnerName, sizeof(sharedOwnerName));
            JsonGetString(sharedInfoStart, "owner_email", sharedOwnerEmail, sizeof(sharedOwnerEmail));
            JsonGetString(sharedInfoStart, "shared_date", sharedDate, sizeof(sharedDate));
        }
    }
    
    // Store values if login successful
    if (strcmp(status, "active") == 0) {
        m_licStatus = LIC_VALID;
        m_bLicensed = true;
        wcscpy_s(m_szUserEmail, email);
        strcpy_s(m_szLicenseKey, licenseKey);
        strcpy_s(m_szAuthToken, authToken);
        strcpy_s(m_szLicExpiry, expiry);
        strcpy_s(m_szTokenExpiry, tokenExpiry);
        strcpy_s(m_szLicenseType, licenseType);
        m_nPluginId = pluginId;
        m_nMaxActivations = maxActivations;
        m_nCurrentActivations = currentActivations;
        m_nMaxSubLicenses = maxSubLicenses;
        m_nCurrentSubLicenses = currentSubLicenses;
        m_bIsSharedLicense = isShared;
        strcpy_s(m_szSharedOwnerName, sharedOwnerName);
        strcpy_s(m_szSharedOwnerEmail, sharedOwnerEmail);
        strcpy_s(m_szSharedDate, sharedDate);
        
        // Save to secure storage
        LicenseSaveSecureCredentials();
        LicenseSaveToRegistry();
    }
}
```

---

## 3. UI Changes

### File: `TellyMediaUI.cpp` (or your plugin's UI file)

#### A. Status Section Layout

Create a compact multi-column layout:

```cpp
// -- Status Info --
HWND hInfoHdr = MkLabel(p, L"Status", M, y, 200, 18);
SendMessageW(hInfoHdr, WM_SETFONT, (WPARAM)g_hFontBold, FALSE);
y += 26;

// Define column positions
int col1 = M;
int col2 = M + 120;
int col3 = M + 200;
int col4 = M + 310;
int col5 = M + 410;

// Row 1: License Status | Main Account | Owner Email (if sub-license)
MkLabel(p, L"License Status:", col1, y, 110, 16);
{
    const wchar_t* statusText = L"Not Activated";
    if (d && d->plugin) {
        switch (d->plugin->m_licStatus) {
            case YourPlugin::LIC_VALID:    statusText = L"Active"; break;
            case YourPlugin::LIC_EXPIRED:  statusText = L"Expired"; break;
            case YourPlugin::LIC_INVALID:  statusText = L"Invalid"; break;
            case YourPlugin::LIC_REVOKED:  statusText = L"Revoked"; break;
            case YourPlugin::LIC_ERROR:    statusText = L"Server Error"; break;
            default: break;
        }
    }
    MkLabel(p, statusText, col2, y, 75, 16);
}

// Show Owner Email if sub-license
if (d && d->plugin && d->plugin->m_bIsSharedLicense) {
    MkLabel(p, L"Owner Email:", col3, y, 95, 16);
    wchar_t ownerEmailW[128] = L"N/A";
    if (d->plugin->m_szSharedOwnerEmail[0])
        MultiByteToWideChar(CP_UTF8, 0, d->plugin->m_szSharedOwnerEmail, -1, ownerEmailW, 128);
    MkLabel(p, ownerEmailW, col4, y, 180, 16);
}
y += 20;

// Row 2: Expires | License Type
MkLabel(p, L"Expires:", col1, y, 110, 16);
{
    wchar_t expW[64] = L"N/A";
    if (d && d->plugin) {
        if (d->plugin->m_szLicenseType[0] && 
            (_stricmp(d->plugin->m_szLicenseType, "lifetime") == 0 || 
             _stricmp(d->plugin->m_szLicenseType, "Lifetime") == 0)) {
            wcscpy_s(expW, L"Unlimited");
        } else if (d->plugin->m_szLicExpiry[0]) {
            ConvertToUKDate(d->plugin->m_szLicExpiry, expW, 64);
        }
    }
    MkLabel(p, expW, col2, y, 75, 16);
}

MkLabel(p, L"License Type:", col3, y, 100, 16);
{
    wchar_t typeW[64] = L"N/A";
    if (d && d->plugin && d->plugin->m_szLicenseType[0])
        MultiByteToWideChar(CP_UTF8, 0, d->plugin->m_szLicenseType, -1, typeW, 64);
    MkLabel(p, typeW, col4, y, 100, 16);
}
y += 20;

// Row 3: Activations | Session Expires
MkLabel(p, L"Activations:", col1, y, 110, 16);
{
    wchar_t activationsW[64] = L"N/A";
    if (d && d->plugin) {
        swprintf_s(activationsW, L"%d / %d", d->plugin->m_nCurrentActivations, d->plugin->m_nMaxActivations);
    }
    MkLabel(p, activationsW, col2, y, 75, 16);
}

if (d && d->plugin && d->plugin->m_szTokenExpiry[0]) {
    MkLabel(p, L"Session Expires:", col3, y, 105, 16);
    wchar_t tokenExpiryW[64] = L"N/A";
    MultiByteToWideChar(CP_UTF8, 0, d->plugin->m_szTokenExpiry, -1, tokenExpiryW, 64);
    MkLabel(p, tokenExpiryW, col4, y, 150, 16);
}
y += 20;

// Row 4: Sub-Licenses (only show if user has sub-license capability)
if (d && d->plugin && d->plugin->m_nMaxSubLicenses > 0) {
    MkLabel(p, L"Sub-Licenses:", col1, y, 110, 16);
    wchar_t subLicensesW[64] = L"N/A";
    swprintf_s(subLicensesW, L"%d / %d", d->plugin->m_nCurrentSubLicenses, d->plugin->m_nMaxSubLicenses);
    MkLabel(p, subLicensesW, col2, y, 75, 16);
    y += 20;
}

y += 12;
```

#### B. Sub-License Information Section

Add styled orange box with sub-license details:

```cpp
// Shared license info (shown only if license is shared)
if (d && d->plugin && d->plugin->m_bIsSharedLicense && d->plugin->m_szSharedOwnerName[0]) {
    // Draw a nice rounded rectangle with gradient for the orange box
    HDC hdc = GetDC(p);
    int boxX = DPI(M - 5);
    int boxY = DPI(y - 5);
    int boxW = DPI(570);
    int boxH = DPI(85);
    
    // Create rounded rectangle region
    HRGN hRgn = CreateRoundRectRgn(boxX, boxY, boxX + boxW, boxY + boxH, DPI(8), DPI(8));
    
    // Fill with orange gradient (lighter at top, darker at bottom)
    RECT gradRect = {boxX, boxY, boxX + boxW, boxY + boxH};
    TRIVERTEX vertex[2];
    vertex[0].x = gradRect.left;
    vertex[0].y = gradRect.top;
    vertex[0].Red = 0xFF00;    // RGB(255, 160, 0) - lighter orange
    vertex[0].Green = 0xA000;
    vertex[0].Blue = 0x0000;
    vertex[0].Alpha = 0x0000;
    
    vertex[1].x = gradRect.right;
    vertex[1].y = gradRect.bottom;
    vertex[1].Red = 0xFF00;    // RGB(255, 120, 0) - darker orange
    vertex[1].Green = 0x7800;
    vertex[1].Blue = 0x0000;
    vertex[1].Alpha = 0x0000;
    
    GRADIENT_RECT gRect = {0, 1};
    SelectClipRgn(hdc, hRgn);
    GradientFill(hdc, vertex, 2, &gRect, 1, GRADIENT_FILL_RECT_V);
    SelectClipRgn(hdc, nullptr);
    
    // Draw subtle border
    HPEN hBorderPen = CreatePen(PS_SOLID, 1, RGB(200, 100, 0));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hBorderPen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    RoundRect(hdc, boxX, boxY, boxX + boxW, boxY + boxH, DPI(8), DPI(8));
    SelectObject(hdc, hOldPen);
    SelectObject(hdc, hOldBrush);
    DeleteObject(hBorderPen);
    DeleteObject(hRgn);
    ReleaseDC(p, hdc);
    
    // Section header
    HWND hSharedHdr = CreateWindowExW(0, L"STATIC", L"Sub-License Information",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        DPI(M + 5), DPI(y + 2), DPI(300), 18,
        p, nullptr, hInst, nullptr);
    SendMessageW(hSharedHdr, WM_SETFONT, (WPARAM)g_hFontBold, FALSE);
    y += 26;
    
    // Row 1: Sub Account Name (sub-license user's name)
    MkLabel(p, L"Sub Account Name:", M + 10, y, 130, 16);
    wchar_t subUserName[128] = L"";
    if (d->plugin->m_szUserEmail[0]) {
        wcscpy_s(subUserName, d->plugin->m_szUserEmail);
        wchar_t* atSign = wcschr(subUserName, L'@');
        if (atSign) *atSign = L'\0'; // Extract username from email
    }
    MkLabel(p, subUserName, M + 145, y, 200, 16);
    y += 20;
    
    // Row 2: Sub Account Email
    MkLabel(p, L"Sub Account Email:", M + 10, y, 130, 16);
    wchar_t subUserEmailW[256] = L"";
    if (d->plugin->m_szUserEmail[0])
        wcscpy_s(subUserEmailW, d->plugin->m_szUserEmail);
    MkLabel(p, subUserEmailW, M + 145, y, 400, 16);
    y += 20;
    
    // Row 3: Shared On date
    if (d->plugin->m_szSharedDate[0]) {
        MkLabel(p, L"Shared On:", M + 10, y, 90, 16);
        wchar_t sharedDateW[64] = L"";
        MultiByteToWideChar(CP_UTF8, 0, d->plugin->m_szSharedDate, -1, sharedDateW, 64);
        MkLabel(p, sharedDateW, M + 100, y, 200, 16);
        y += 20;
    }
    
    y += 12; // Extra spacing after shared info section
}
```

---

## 4. Server-Side Changes

### File: `login.php`

#### A. Add Token Expiration Column to Database

```sql
ALTER TABLE licenses 
ADD COLUMN token_expires_at DATETIME DEFAULT NULL;
```

#### B. Update Login Logic

```php
// Check if user is a sub-license user first
$subLicense = Database::fetch("SELECT * FROM sub_licenses WHERE email = ? AND status = 'active'", [$email]);
$isSubLicenseUser = false;

if ($subLicense) {
    // Use sub-license data for authentication
    $user = [
        'id' => $subLicense['id'],
        'email' => $subLicense['email'],
        'name' => $subLicense['name'] ?? 'Sub-License User',
        'password' => $subLicense['password'],
        'is_active' => ($subLicense['status'] === 'active') ? 1 : 0
    ];
    $isSubLicenseUser = true;
} else {
    // Fallback to users table
    $user = Database::fetch("SELECT * FROM users WHERE email = ? AND is_active = 1", [$email]);
}

// ... verify password ...

// Get license info
if ($isSubLicenseUser) {
    // For sub-license users, get the parent license
    $license = Database::fetch(
        "SELECT l.* FROM licenses l 
         INNER JOIN sub_licenses sl ON l.user_id = sl.owner_user_id 
         WHERE sl.email = ? AND sl.status = 'active'",
        [$email]
    );
    
    // Get shared info
    $sharedInfo = Database::fetch(
        "SELECT sl.created_at as shared_date, u.name as owner_name, u.email as owner_email
         FROM sub_licenses sl
         INNER JOIN users u ON sl.owner_user_id = u.id
         WHERE sl.email = ?",
        [$email]
    );
} else {
    $license = Database::fetch("SELECT * FROM licenses WHERE user_id = ? AND plugin_id = ?", [$user['id'], $pluginId]);
}

// Generate auth token with 14-day expiration
$tokenExpires = date('Y-m-d H:i:s', strtotime('+14 days'));
$authToken = bin2hex(random_bytes(32));

// Update license with token and expiration
Database::execute(
    "UPDATE licenses SET auth_token = ?, token_expires_at = ? WHERE id = ?",
    [$authToken, $tokenExpires, $license['id']]
);

// Count sub-licenses
$subLicenseCount = Database::fetch(
    "SELECT COUNT(*) as count FROM sub_licenses WHERE owner_user_id = ? AND status = 'active'",
    [$license['user_id']]
);

// Build response
$response = [
    'status' => 'active',
    'message' => 'Login successful',
    'license_key' => $license['license_key'],
    'auth_token' => $authToken,
    'expires' => $license['expires_at'],
    'token_expires' => $tokenExpires,
    'license_type' => $license['license_type'],
    'plugin_id' => $license['plugin_id'],
    'max_activations' => $license['max_activations'],
    'current_activations' => $license['current_activations'],
    'max_sub_licenses' => $license['max_sub_licenses'] ?? 0,
    'current_sub_licenses' => $subLicenseCount['count'] ?? 0,
    'is_shared' => $isSubLicenseUser
];

if ($isSubLicenseUser && $sharedInfo) {
    $response['shared_info'] = [
        'owner_name' => $sharedInfo['owner_name'],
        'owner_email' => $sharedInfo['owner_email'],
        'shared_date' => $sharedInfo['shared_date']
    ];
}

echo json_encode($response);
```

### File: `verify-license.php`

Add token expiration check:

```php
// Check if token has expired
if ($license['token_expires_at'] && strtotime($license['token_expires_at']) < time()) {
    echo json_encode([
        'status' => 'token_expired',
        'message' => 'Session expired. Please login again.'
    ]);
    exit;
}
```

---

## 5. Step-by-Step Implementation Guide

### For YouTube and SoundCloud Plugins:

#### Step 1: Update Plugin Header
1. Copy all licensing member variables from section 1
2. Add to your plugin's header file (e.g., `YouTubePlugin.h`)
3. Ensure enum `LicenseStatus` is defined

#### Step 2: Update Plugin Constructor
1. Initialize all licensing fields to empty/zero values
2. Copy initialization code from section 2A

#### Step 3: Update LicenseLogin Function
1. Add parsing for all new fields (token_expires, sub-license counts, shared info)
2. Copy parsing code from section 2B
3. Store all values in member variables

#### Step 4: Update UI Layout
1. Replace old license panel with new multi-column Status section
2. Add Sub-License Information section with orange gradient box
3. Copy UI code from section 3A and 3B
4. Adjust column positions and spacing as needed for your UI

#### Step 5: Update Server-Side
1. Add `token_expires_at` column to licenses table
2. Update `login.php` to check sub_licenses table first
3. Return all required fields in JSON response
4. Add token expiration check to `verify-license.php`

#### Step 6: Test
1. Test normal user login
2. Test sub-license user login
3. Verify Status section displays all fields correctly
4. Verify Sub-License Information box appears for sub-users
5. Test token expiration (14 days)
6. Verify "Unlimited" shows for lifetime licenses

---

## Key Features Implemented

✅ **14-day token expiration** - Forces re-login after 14 days for security  
✅ **Sub-license support** - Users can share licenses with sub-accounts  
✅ **Multi-column Status layout** - Compact, professional display  
✅ **Sub-License Information box** - Styled orange gradient box with rounded corners  
✅ **Sub-license count tracking** - Shows "1 / 5" active sub-licenses  
✅ **Lifetime license handling** - Shows "Unlimited" instead of "N/A"  
✅ **Session expiration display** - Shows when token expires  
✅ **Owner email display** - Sub-users see who shared the license  

---

## Support Email Update

Don't forget to update the support email in your UI:

```cpp
MkLabel(p, L"Support: support@djeventsuite.cloud", M, y, 400, 16);
```

---

## Visual Layout Examples

### Status Section (Normal User)
```
Status
License Status: Active          License Type: lifetime
Expires: Unlimited              Session Expires: 2026-03-25
Activations: 2 / 4
Sub-Licenses: 1 / 5
```

### Status Section (Sub-License User)
```
Status
License Status: Active          Owner Email: admin@example.com
Expires: Unlimited              License Type: lifetime
Activations: 2 / 4              Session Expires: 2026-03-25
```

### Sub-License Information Box
```
┌─────────────────────────────────────────────────────┐
│ Sub-License Information                             │
│ Sub Account Name:    mk                             │
│ Sub Account Email:   mk@example.com                 │
│ Shared On:           2026-03-11 20:09:07            │
└─────────────────────────────────────────────────────┘
(Orange gradient background with rounded corners)
```

---

This documentation covers all changes needed to replicate the TellyMedia license manager in your YouTube and SoundCloud plugins. Follow the steps in order and test thoroughly at each stage.
