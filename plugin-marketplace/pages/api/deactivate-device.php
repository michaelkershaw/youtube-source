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

if (empty($licenseKey) || empty($deviceId)) {
    http_response_code(400);
    echo json_encode(['status' => 'error', 'message' => 'license_key and device_id are required.']);
    exit;
}

$license = Database::fetch("SELECT * FROM licenses WHERE license_key = ?", [$licenseKey]);
if (!$license) {
    http_response_code(404);
    echo json_encode(['status' => 'error', 'message' => 'License key not found.']);
    exit;
}

$device = Database::fetch(
    "SELECT * FROM device_activations WHERE license_id = ? AND device_id = ? AND is_active = 1",
    [$license['id'], $deviceId]
);

if (!$device) {
    echo json_encode(['status' => 'error', 'message' => 'Device not found or already deactivated.']);
    exit;
}

Database::update('device_activations', [
    'is_active' => 0,
    'deactivated_at' => date('Y-m-d H:i:s'),
], 'id = ?', [$device['id']]);

Database::query("UPDATE licenses SET current_activations = GREATEST(current_activations - 1, 0) WHERE id = ?", [$license['id']]);

echo json_encode([
    'status' => 'success',
    'message' => 'Device deactivated successfully.',
    'current_activations' => max(0, $license['current_activations'] - 1),
    'max_activations' => (int)$license['max_activations'],
]);
