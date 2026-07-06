<?php
require_login();
header('Content-Type: application/json');

$upgradeId = (int)($_POST['upgrade_id'] ?? 0);
$paymentMethod = $_POST['payment_method'] ?? '';
$transactionId = $_POST['transaction_id'] ?? '';

// Get upgrade record
$upgrade = Database::fetch(
    "SELECT dsu.*, l.license_key, l.max_activations, l.devices, l.upgrade_history
     FROM device_slots_upgrades dsu
     JOIN licenses l ON dsu.license_id = l.id
     WHERE dsu.id = ? AND dsu.status = 'pending'",
    [$upgradeId]
);

if (!$upgrade) {
    http_response_code(400);
    echo json_encode(['success' => false, 'error' => 'Invalid upgrade request']);
    exit;
}

// Verify license ownership
if ($upgrade['user_id'] !== current_user_id()) {
    http_response_code(403);
    echo json_encode(['success' => false, 'error' => 'Unauthorized']);
    exit;
}

// Update license with additional slots
$newMaxSlots = $upgrade['max_activations'] + $upgrade['additional_slots'];

// Update upgrade history
$history = $upgrade['upgrade_history'] ? json_decode($upgrade['upgrade_history'], true) : [];
$history[] = [
    'date' => date('Y-m-d H:i:s'),
    'slots' => $upgrade['additional_slots'],
    'price' => $upgrade['price'],
    'previous_max' => $upgrade['max_activations'],
    'new_max' => $newMaxSlots
];

// Update license
Database::update('licenses', [
    'max_activations_updated' => $newMaxSlots,
    'upgrade_history' => json_encode($history),
    'updated_at' => date('Y-m-d H:i:s')
], 'id = ?', [$upgrade['license_id']]);

// Mark upgrade as completed
Database::update('device_slots_upgrades', [
    'status' => 'completed',
    'payment_method' => $paymentMethod,
    'transaction_id' => $transactionId,
    'paid_at' => date('Y-m-d H:i:s'),
    'updated_at' => date('Y-m-d H:i:s')
], 'id = ?', [$upgradeId]);

echo json_encode([
    'success' => true,
    'message' => 'Device slots upgraded successfully!',
    'new_max_slots' => $newMaxSlots,
    'additional_slots' => $upgrade['additional_slots']
]);
