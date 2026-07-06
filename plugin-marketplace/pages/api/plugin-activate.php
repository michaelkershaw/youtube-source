<?php
/**
 * Plugin License Activation API
 * Compatible with VirtualDJ C++ plugins (SoundCloud Source, YouTube Source, etc.)
 * 
 * POST Parameters:
 * - key: License key (format: XX-XXXX-XXXX-XXXX-XXXX-XXXX)
 * - machine_id: Hardware fingerprint (8-char hex)
 * - computer_name: Computer name (optional)
 * 
 * Returns JSON:
 * - status: active|expired|revoked|machine_mismatch|invalid|error
 * - message: Human-readable message
 * - expiry: Expiry date (YYYY-MM-DD HH:MM:SS) or null for lifetime
 * - server_time: Current server time
 * - auth_token: Token for device management (on success)
 */

header('Content-Type: application/json');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: POST');

require_once __DIR__ . '/../../includes/db.php';

function jsonResponse($data, $httpCode = 200) {
    http_response_code($httpCode);
    echo json_encode($data);
    exit;
}

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    jsonResponse(['status' => 'error', 'message' => 'POST required', 'server_time' => date('Y-m-d H:i:s')], 405);
}

$key = trim($_POST['key'] ?? '');
$machineId = trim($_POST['machine_id'] ?? '');
$computerName = trim($_POST['computer_name'] ?? 'Unknown');

if (empty($key) || empty($machineId)) {
    jsonResponse(['status' => 'error', 'message' => 'Missing key or machine_id', 'server_time' => date('Y-m-d H:i:s')], 400);
}

// Validate key format: XX-XXXX-XXXX-XXXX-XXXX-XXXX (TM, YT, SC, etc.)
if (!preg_match('/^[A-Z]{2}-[A-Z0-9]{4}-[A-Z0-9]{4}-[A-Z0-9]{4}-[A-Z0-9]{4}-[A-Z0-9]{4}$/i', $key)) {
    jsonResponse(['status' => 'error', 'message' => 'Invalid key format', 'server_time' => date('Y-m-d H:i:s')], 400);
}

// Look up the license with plugin validation
$license = Database::fetch(
    "SELECT l.*, p.name as plugin_name, p.slug as plugin_slug 
     FROM licenses l
     LEFT JOIN plugins p ON l.plugin_id = p.id
     WHERE l.license_key = ? AND (p.slug = ? OR ? IS NULL)",
    [$key, $pluginId, $pluginId]
);

if (!$license) {
    jsonResponse(['status' => 'invalid', 'message' => 'License key not found', 'server_time' => date('Y-m-d H:i:s')]);
}

// Check if revoked
if ($license['status'] === 'revoked') {
    jsonResponse(['status' => 'revoked', 'message' => 'This license has been revoked', 'server_time' => date('Y-m-d H:i:s')]);
}

// Check if expired
if ($license['expires_at'] && strtotime($license['expires_at']) < time()) {
    Database::update('licenses', ['status' => 'expired'], 'id = ?', [$license['id']]);
    jsonResponse([
        'status' => 'expired',
        'message' => 'This license has expired',
        'expiry' => $license['expires_at'],
        'server_time' => date('Y-m-d H:i:s')
    ]);
}

// Check if device is already activated
$existingDevice = Database::fetch(
    "SELECT * FROM device_activations WHERE license_id = ? AND device_id = ?",
    [$license['id'], $machineId]
);

$now = date('Y-m-d H:i:s');

if ($existingDevice) {
    // Device already activated - update last seen
    Database::update('device_activations', [
        'last_seen_at' => $now,
        'device_name' => $computerName,
        'is_active' => 1
    ], 'id = ?', [$existingDevice['id']]);
    
    jsonResponse([
        'status' => 'active',
        'message' => 'Device already activated',
        'expiry' => $license['expires_at'],
        'auth_token' => $license['auth_token'],
        'machine_id' => $machineId,
        'server_time' => $now
    ]);
}

// Check max activations
$currentActivations = Database::count('device_activations', 'license_id = ? AND is_active = 1', [$license['id']]);
$maxActivations = $license['max_activations_updated'] ?? $license['max_activations'];

if ($currentActivations >= $maxActivations) {
    jsonResponse([
        'status' => 'max_devices',
        'message' => "Maximum device activations reached ({$currentActivations}/{$maxActivations})",
        'current_activations' => (int)$currentActivations,
        'max_activations' => (int)$maxActivations,
        'server_time' => $now
    ], 403);
}

// Generate auth token if not exists
if (empty($license['auth_token'])) {
    $authToken = bin2hex(random_bytes(32));
    Database::update('licenses', ['auth_token' => $authToken], 'id = ?', [$license['id']]);
} else {
    $authToken = $license['auth_token'];
}

// Activate new device
Database::insert('device_activations', [
    'license_id' => $license['id'],
    'device_id' => $machineId,
    'device_name' => $computerName,
    'ip_address' => $_SERVER['REMOTE_ADDR'] ?? null,
    'is_active' => 1,
    'activated_at' => $now,
    'last_seen_at' => $now
]);

// Update device count
$newCount = Database::count('device_activations', 'license_id = ? AND is_active = 1', [$license['id']]);
Database::update('licenses', [
    'devices' => $newCount,
    'last_activated_at' => $now
], 'id = ?', [$license['id']]);

jsonResponse([
    'status' => 'active',
    'message' => 'License activated successfully',
    'expiry' => $license['expires_at'],
    'auth_token' => $authToken,
    'machine_id' => $machineId,
    'current_activations' => (int)$newCount,
    'max_activations' => (int)$maxActivations,
    'server_time' => $now
]);
    'auth_token' => $authToken,
    'machine_id' => $machineId,
    'current_activations' => (int)$newCount,
    'max_activations' => (int)$maxActivations,
    'server_time' => $now
]);
