<?php
header('Content-Type: application/json');

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    echo json_encode(['error' => 'Method not allowed. Use POST.']);
    exit;
}

$input = json_decode(file_get_contents('php://input'), true);
if (!$input) $input = $_POST;

$licenseKey = trim($input['license_key'] ?? '');
$deviceId = trim($input['device_id'] ?? '');
$deviceName = trim($input['device_name'] ?? '');

if (empty($licenseKey)) {
    http_response_code(400);
    echo json_encode(['status' => 'error', 'message' => 'license_key is required.']);
    exit;
}

$license = Database::fetch(
    "SELECT l.*, p.name as plugin_name, p.slug as plugin_slug, p.identifier as plugin_identifier
     FROM licenses l JOIN plugins p ON l.plugin_id = p.id
     WHERE l.license_key = ?",
    [$licenseKey]
);

if (!$license) {
    http_response_code(404);
    echo json_encode(['status' => 'invalid', 'message' => 'License key not found.']);
    exit;
}

// Check if expired
if ($license['expires_at'] && strtotime($license['expires_at']) < time()) {
    if ($license['status'] === 'active') {
        Database::update('licenses', ['status' => 'expired'], 'id = ?', [$license['id']]);
    }
    echo json_encode([
        'status' => 'expired',
        'plugin' => $license['plugin_name'],
        'plugin_id' => (int)$license['plugin_id'],
        'expires' => $license['expires_at'] ? date('Y-m-d', strtotime($license['expires_at'])) : null,
        'user_id' => (int)$license['user_id'],
        'message' => 'License has expired.',
    ]);
    exit;
}

// Check if revoked
if ($license['status'] === 'revoked') {
    echo json_encode([
        'status' => 'revoked',
        'plugin' => $license['plugin_name'],
        'user_id' => (int)$license['user_id'],
        'message' => 'License has been revoked.',
    ]);
    exit;
}

// Handle device activation
if (!empty($deviceId)) {
    $existing = Database::fetch(
        "SELECT * FROM device_activations WHERE license_id = ? AND device_id = ?",
        [$license['id'], $deviceId]
    );

    if ($existing) {
        // Update last seen
        Database::update('device_activations', [
            'last_seen_at' => date('Y-m-d H:i:s'),
            'ip_address' => $_SERVER['REMOTE_ADDR'] ?? '',
        ], 'id = ?', [$existing['id']]);
    } else {
        // Check activation limit
        if ($license['devices'] >= $license['max_activations']) {
            echo json_encode([
                'status' => 'activation_limit',
                'plugin' => $license['plugin_name'],
                'max_activations' => (int)$license['max_activations'],
                'current_activations' => (int)$license['devices'],
                'message' => 'Maximum device activation limit reached. Deactivate a device first.',
            ]);
            exit;
        }

        // Create new activation
        Database::insert('device_activations', [
            'license_id' => $license['id'],
            'device_id' => $deviceId,
            'device_name' => $deviceName ?: null,
            'ip_address' => $_SERVER['REMOTE_ADDR'] ?? '',
            'is_active' => 1,
            'activated_at' => date('Y-m-d H:i:s'),
            'last_seen_at' => date('Y-m-d H:i:s'),
            'created_at' => date('Y-m-d H:i:s'),
        ]);

        Database::query("UPDATE licenses SET devices = devices + 1, last_activated_at = NOW() WHERE id = ?", [$license['id']]);
        $license['devices']++;
    }
}

echo json_encode([
    'status' => 'active',
    'plugin' => $license['plugin_name'],
    'plugin_id' => (int)$license['plugin_id'],
    'expires' => $license['expires_at'] ? date('Y-m-d', strtotime($license['expires_at'])) : null,
    'license_type' => $license['license_type'],
    'user_id' => (int)$license['user_id'],
    'max_activations' => (int)$license['max_activations'],
    'current_activations' => (int)($license['devices'] ?? 0),
]);
