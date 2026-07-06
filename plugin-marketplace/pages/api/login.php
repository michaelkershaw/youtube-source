<?php
header('Content-Type: application/json');

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    echo json_encode(['error' => 'Method not allowed. Use POST.']);
    exit;
}

$input = json_decode(file_get_contents('php://input'), true);
if (!$input) $input = $_POST;

$email = trim($input['email'] ?? '');
$password = trim($input['password'] ?? '');
$pluginIdentifier = trim($input['plugin'] ?? '');

if (empty($email) || empty($password) || empty($pluginIdentifier)) {
    http_response_code(400);
    echo json_encode(['status' => 'error', 'message' => 'email, password, and plugin are required.']);
    exit;
}

// Find user by email
$user = Database::fetch("SELECT * FROM users WHERE email = ? AND is_active = 1", [$email]);
if (!$user) {
    error_log("LOGIN DEBUG: User not found for email: $email");
    http_response_code(401);
    echo json_encode(['status' => 'error', 'message' => 'Invalid email or password.']);
    exit;
}
if (!password_verify($password, $user['password'])) {
    error_log("LOGIN DEBUG: Password mismatch for email: $email");
    http_response_code(401);
    echo json_encode(['status' => 'error', 'message' => 'Invalid email or password.']);
    exit;
}

// Find plugin by identifier
$plugin = Database::fetch("SELECT * FROM plugins WHERE identifier = ? AND is_active = 1", [$pluginIdentifier]);
if (!$plugin) {
    error_log("LOGIN DEBUG: Plugin not found for identifier: $pluginIdentifier");
    http_response_code(404);
    echo json_encode(['status' => 'error', 'message' => 'Plugin not found.']);
    exit;
}
error_log("LOGIN DEBUG: Found plugin: {$plugin['name']} (ID: {$plugin['id']}) for user: {$user['email']} (ID: {$user['id']})");

// Find user's active license for this plugin
$license = Database::fetch(
    "SELECT l.*, lf.prefix, lf.format_pattern, lf.separator
     FROM licenses l 
     LEFT JOIN license_formats lf ON lf.plugin_id = l.plugin_id AND lf.is_active = 1
     WHERE l.user_id = ? AND l.plugin_id = ? AND l.status = 'active'",
    [$user['id'], $plugin['id']]
);

if (!$license) {
    error_log("LOGIN DEBUG: No active license found for user {$user['id']} and plugin {$plugin['id']}");
    // Let's check what licenses this user has
    $allLicenses = Database::fetchAll("SELECT * FROM licenses WHERE user_id = ?", [$user['id']]);
    error_log("LOGIN DEBUG: User has " . count($allLicenses) . " total licenses");
    foreach ($allLicenses as $lic) {
        error_log("LOGIN DEBUG: License ID {$lic['id']}: plugin_id={$lic['plugin_id']}, status={$lic['status']}");
    }
    http_response_code(404);
    echo json_encode(['status' => 'error', 'message' => 'No active license found for this plugin.']);
    exit;
}
error_log("LOGIN DEBUG: Found active license: {$license['license_key']} for plugin {$plugin['id']}");

// Check if expired
if ($license['expires_at'] && strtotime($license['expires_at']) < time()) {
    Database::update('licenses', ['status' => 'expired'], 'id = ?', [$license['id']]);
    echo json_encode([
        'status' => 'expired',
        'message' => 'License has expired.',
        'expires' => $license['expires_at'] ? date('Y-m-d', strtotime($license['expires_at'])) : null
    ]);
    exit;
}

// Generate auth token (random 64-char string)
$authToken = bin2hex(random_bytes(32));

// Get device name from request
$deviceName = trim($input['device_name'] ?? 'Plugin Device');
if (empty($deviceName)) {
    $deviceName = 'Plugin Device';
}

// Generate device ID from client info (fallback if not provided)
$deviceId = $_SERVER['HTTP_USER_AGENT'] ?? 'Unknown-Device';
$deviceId = substr(md5($deviceId . ($_SERVER['REMOTE_ADDR'] ?? '')), 0, 16);

error_log("LOGIN DEBUG: Generated device ID: $deviceId for license {$license['id']}");
error_log("LOGIN DEBUG: Using device name: $deviceName");
file_put_contents(__DIR__ . '/../../debug_login.log', date('Y-m-d H:i:s') . " - Generated device ID: $deviceId for license {$license['id']} (Device: $deviceName)\n", FILE_APPEND);
error_log("LOGIN DEBUG: Current devices value: " . ($license['devices'] ?? 'NULL'));
error_log("LOGIN DEBUG: Max activations: " . ($license['max_activations'] ?? 'NULL'));

// Check if device is already activated
try {
    $existingDevice = Database::fetch(
        "SELECT * FROM device_activations WHERE license_id = ? AND device_id = ?",
        [$license['id'], $deviceId]
    );
    error_log("LOGIN DEBUG: Existing device found: " . ($existingDevice ? 'YES (ID: ' . $existingDevice['id'] . ')' : 'NO'));
} catch (Exception $e) {
    error_log("LOGIN DEBUG ERROR: Failed to check existing device: " . $e->getMessage());
    $existingDevice = null;
}

if (!$existingDevice) {
    // Check activation limit
    $currentActivations = $license['devices'] ?? 0;
    $maxActivations = $license['max_activations'] ?? 1;
    
    error_log("LOGIN DEBUG: Checking limit - current: $currentActivations, max: $maxActivations");
    
    if ($currentActivations < $maxActivations) {
        // Activate new device
        try {
            $insertData = [
                'license_id' => $license['id'],
                'device_id' => $deviceId,
                'device_name' => $deviceName,
                'ip_address' => $_SERVER['REMOTE_ADDR'] ?? null,
                'is_active' => 1,
                'activated_at' => date('Y-m-d H:i:s'),
                'last_seen_at' => date('Y-m-d H:i:s')
            ];
            error_log("LOGIN DEBUG: Attempting to insert device: " . json_encode($insertData));
            
            $insertId = Database::insert('device_activations', $insertData);
            error_log("LOGIN DEBUG: New device inserted with ID: $insertId");
        } catch (Exception $e) {
            error_log("LOGIN DEBUG ERROR: Failed to insert device: " . $e->getMessage());
        }
    } else {
        error_log("LOGIN DEBUG: Device activation limit reached for license {$license['id']} ($currentActivations/$maxActivations)");
    }
} else {
    // Update last seen and ensure device is active
    try {
        Database::update('device_activations', [
            'last_seen_at' => date('Y-m-d H:i:s'),
            'is_active' => 1
        ], 'id = ?', [$existingDevice['id']]);
        error_log("LOGIN DEBUG: Updated last_seen_at and set is_active=1 for existing device ID: {$existingDevice['id']}");
    } catch (Exception $e) {
        error_log("LOGIN DEBUG ERROR: Failed to update device: " . $e->getMessage());
    }
}

// Always sync devices count with actual device_activations count
try {
    $actualDeviceCount = Database::count('device_activations', 'license_id = ? AND is_active = 1', [$license['id']]);
    error_log("LOGIN DEBUG: Counted $actualDeviceCount active devices for license {$license['id']}");
    
    Database::update('licenses', [
        'devices' => $actualDeviceCount
    ], 'id = ?', [$license['id']]);
    error_log("LOGIN DEBUG: Updated licenses.devices to $actualDeviceCount for license {$license['id']}");
} catch (Exception $e) {
    error_log("LOGIN DEBUG ERROR: Failed to sync device count: " . $e->getMessage());
    $actualDeviceCount = 0;
}

// Update license with auth token and last login
Database::update('licenses', [
    'auth_token' => $authToken,
    'last_login_at' => date('Y-m-d H:i:s')
], 'id = ?', [$license['id']]);

// Get updated activation count from licenses table (may have been updated above)
$updatedLicense = Database::fetch("SELECT devices, max_activations FROM licenses WHERE id = ?", [$license['id']]);
$currentActivations = $updatedLicense['devices'] ?? 0;
$maxActivations = $updatedLicense['max_activations'] ?? 1;

// Format license key if needed
$licenseKey = $license['license_key'];
if ($license['prefix'] && !str_starts_with($licenseKey, $license['prefix'])) {
    $licenseKey = $license['prefix'] . '-' . $licenseKey;
}

// Determine license type from pricing
$licenseType = 'unknown';
$pricing = Database::fetch("SELECT license_type FROM plugin_pricings WHERE plugin_id = ? AND is_active = 1 ORDER BY price ASC LIMIT 1", [$plugin['id']]);
if ($pricing) {
    $licenseType = $pricing['license_type'];
}

$response = [
    'status' => 'active',
    'message' => 'Login successful.',
    'license_key' => $licenseKey,
    'auth_token' => $authToken,
    'expires' => $license['expires_at'] ? date('Y-m-d', strtotime($license['expires_at'])) : null,
    'license_type' => $licenseType,
    'plugin' => $plugin['name'],
    'plugin_id' => (int)$plugin['id'],
    'user_id' => (int)$user['id'],
    'max_activations' => (int)$maxActivations,
    'current_activations' => (int)$currentActivations
];

error_log("LOGIN DEBUG: Final response - current_activations: {$currentActivations}, max_activations: {$maxActivations}");
error_log("LOGIN DEBUG: Full response JSON: " . json_encode($response));

echo json_encode($response);
