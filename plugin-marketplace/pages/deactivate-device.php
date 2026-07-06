<?php
require_login();
if ($method !== 'POST') redirect('/dashboard');
require_csrf();

$deviceId = (int)($_GET['device_id'] ?? 0);
$user = current_user();

// Verify the device belongs to this user's license
$device = Database::fetch(
    "SELECT da.*, l.user_id, l.id as license_id
     FROM device_activations da
     JOIN licenses l ON da.license_id = l.id
     WHERE da.id = ? AND l.user_id = ? AND da.is_active = 1",
    [$deviceId, $user['id']]
);

if (!$device) {
    set_flash('error', 'Device not found or already deactivated.');
    redirect('/dashboard');
}

Database::update('device_activations', [
    'is_active' => 0,
    'deactivated_at' => date('Y-m-d H:i:s'),
], 'id = ?', [$device['id']]);

Database::query(
    "UPDATE licenses SET current_activations = GREATEST(current_activations - 1, 0) WHERE id = ?",
    [$device['license_id']]
);

set_flash('success', 'Device deactivated successfully.');
redirect('/dashboard');
