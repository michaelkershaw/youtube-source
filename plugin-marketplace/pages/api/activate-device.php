<?php
header('Content-Type: application/json');

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    echo json_encode(['error' => 'Method not allowed. Use POST.']);
    exit;
}

require_once __DIR__ . '/../../includes/db.php';

$input = json_decode(file_get_contents('php://input'), true);
if (!$input) $input = $_POST;

$licenseKey = trim($input['license_key'] ?? '');
$authToken = trim($input['auth_token'] ?? '');
$deviceId = trim($input['device_id'] ?? '');
$deviceName = trim($input['device_name'] ?? '');

if (empty($licenseKey) || empty($authToken) || empty($deviceId)) {
    http_response_code(400);
    echo json_encode(['status' => 'error', 'message' => 'license_key, auth_token, and device_id are required.']);
    exit;
}

// Find license by key and verify auth token
$license = Database::fetch(
    "SELECT * FROM licenses WHERE license_key = ? AND auth_token = ? AND status = 'active'",
    [$licenseKey, $authToken]
);

if (!$license) {
    http_response_code(401);
    echo json_encode(['status' => 'error', 'message' => 'Invalid license or auth token.']);
    exit;
}

// Check if license is expired
if ($license['expires_at'] && strtotime($license['expires_at']) < time()) {
    http_response_code(403);
    echo json_encode(['status' => 'expired', 'message' => 'License has expired.']);
    exit;
}

// Check if device is already activated
$existingDevice = Database::fetch(
    "SELECT * FROM device_activations WHERE license_id = ? AND device_id = ?",
    [$license['id'], $deviceId]
);

if ($existingDevice) {
    // Device already activated, just update last_seen_at
    Database::update('device_activations', [
        'last_seen_at' => date('Y-m-d H:i:s'),
        'device_name' => $deviceName ?: $existingDevice['device_name']
    ], 'id = ?', [$existingDevice['id']]);
    
    $currentActivations = Database::count('device_activations', 'license_id = ? AND is_active = 1', [$license['id']]);
    
    echo json_encode([
        'status' => 'active',
        'message' => 'Device already activated.',
        'device_id' => $deviceId,
        'current_activations' => (int)$currentActivations,
        'max_activations' => (int)$license['max_activations']
    ]);
    exit;
}

// Check if max activations reached
$currentActivations = Database::count('device_activations', 'license_id = ? AND is_active = 1', [$license['id']]);
$maxActivations = $license['max_activations'] ?? 1;

if ($currentActivations >= $maxActivations) {
    http_response_code(403);
    echo json_encode([
        'status' => 'error',
        'message' => 'Maximum device activations reached.',
        'current_activations' => (int)$currentActivations,
        'max_activations' => (int)$maxActivations
    ]);
    exit;
}

// Activate new device
Database::insert('device_activations', [
    'license_id' => $license['id'],
    'device_id' => $deviceId,
    'device_name' => $deviceName ?: 'Unknown Device',
    'ip_address' => $_SERVER['REMOTE_ADDR'] ?? null,
    'is_active' => 1,
    'activated_at' => date('Y-m-d H:i:s'),
    'last_seen_at' => date('Y-m-d H:i:s')
]);

$currentActivations = Database::count('device_activations', 'license_id = ? AND is_active = 1', [$license['id']]);

// Update devices column in licenses table
Database::update('licenses', [
    'devices' => $currentActivations
], 'id = ?', [$license['id']]);

echo json_encode([
    'status' => 'active',
    'message' => 'Device activated successfully.',
    'device_id' => $deviceId,
    'current_activations' => (int)$currentActivations,
    'max_activations' => (int)$maxActivations
]);
