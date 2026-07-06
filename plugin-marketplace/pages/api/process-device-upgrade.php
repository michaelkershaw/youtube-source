<?php
require_login();
header('Content-Type: application/json');

$licenseId = (int)($_POST['license_id'] ?? 0);
$pricingId = (int)($_POST['pricing_id'] ?? 0);

// Validate license ownership
$license = Database::fetch(
    "SELECT l.*, p.name as plugin_name FROM licenses l 
     JOIN plugins p ON l.plugin_id = p.id
     WHERE l.id = ? AND l.user_id = ? AND l.status = 'active' 
     AND l.license_type = 'lifetime' 
     AND (l.expires_at IS NULL OR l.expires_at > NOW())",
    [$licenseId, current_user_id()]
);

if (!$license) {
    http_response_code(400);
    echo json_encode(['success' => false, 'error' => 'Invalid license']);
    exit;
}

// Get pricing
$pricing = Database::fetch(
    "SELECT * FROM device_slot_pricing WHERE id = ? AND is_active = 1",
    [$pricingId]
);

if (!$pricing) {
    http_response_code(400);
    echo json_encode(['success' => false, 'error' => 'Invalid pricing option']);
    exit;
}

// Generate order number
$orderNumber = 'DS-' . strtoupper(uniqid()) . '-' . date('Y');

// Create upgrade record
$upgradeId = Database::insert('device_slots_upgrades', [
    'license_id' => $licenseId,
    'additional_slots' => $pricing['slots_count'],
    'price' => $pricing['price'],
    'order_number' => $orderNumber,
    'status' => 'pending'
]);

// Return checkout info
echo json_encode([
    'success' => true,
    'order_number' => $orderNumber,
    'amount' => $pricing['price'],
    'description' => "Device Slot Upgrade - +{$pricing['slots_count']} slots for {$license['plugin_name']}",
    'upgrade_id' => $upgradeId
]);
