<?php
/**
 * Plugin License Validation API
 * Compatible with VirtualDJ C++ plugins (SoundCloud Source, YouTube Source, etc.)
 * 
 * POST Parameters:
 * - key: License key
 * - machine_id: Hardware fingerprint (8-char hex)
 * 
 * Returns JSON:
 * - status: active|expired|revoked|machine_mismatch|invalid|error
 * - message: Human-readable message
 * - expiry: Expiry date or null for lifetime
 * - server_time: Current server time
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

if (empty($key) || empty($machineId)) {
    jsonResponse(['status' => 'error', 'message' => 'Missing key or machine_id', 'server_time' => date('Y-m-d H:i:s')], 400);
}

// Look up the license with plugin validation
$license = Database::fetch(
    "SELECT l.*, p.name as plugin_name 
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

// Check if device is activated
$device = Database::fetch(
    "SELECT * FROM device_activations WHERE license_id = ? AND device_id = ? AND is_active = 1",
    [$license['id'], $machineId]
);

if (!$device) {
    jsonResponse([
        'status' => 'machine_mismatch',
        'message' => 'This device is not activated for this license',
        'server_time' => date('Y-m-d H:i:s')
    ]);
}

// Check if expired
if ($license['expires_at'] && strtotime($license['expires_at']) < time()) {
    Database::update('licenses', ['status' => 'expired'], 'id = ?', [$license['id']]);
    jsonResponse([
        'status' => 'expired',
        'message' => 'License has expired',
        'expiry' => $license['expires_at'],
        'server_time' => date('Y-m-d H:i:s')
    ]);
}

// Update last seen
Database::update('device_activations', [
    'last_seen_at' => date('Y-m-d H:i:s')
], 'id = ?', [$device['id']]);

jsonResponse([
    'status' => 'active',
    'message' => 'License is valid',
    'expiry' => $license['expires_at'],
    'machine_id' => $machineId,
    'current_activations' => (int)$license['devices'],
    'max_activations' => (int)($license['max_activations_updated'] ?? $license['max_activations']),
    'server_time' => date('Y-m-d H:i:s')
]);
    'expiry' => $license['expires_at'],
    'machine_id' => $machineId,
    'current_activations' => (int)$license['devices'],
    'max_activations' => (int)($license['max_activations_updated'] ?? $license['max_activations']),
    'server_time' => date('Y-m-d H:i:s')
]);
