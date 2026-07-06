<?php
require_login();
$pageTitle = 'Checkout - Device Slot Upgrade - ' . SITE_NAME;

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    set_flash('error', 'Invalid request method.');
    redirect('/dashboard');
}

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
    set_flash('error', 'Invalid license or you do not own this license.');
    redirect('/dashboard');
}

// Get pricing
$pricing = Database::fetch(
    "SELECT * FROM device_slot_pricing WHERE id = ? AND is_active = 1",
    [$pricingId]
);

if (!$pricing) {
    set_flash('error', 'Invalid pricing option.');
    redirect('/dashboard');
}

// Generate order number
$orderNumber = 'DS-' . strtoupper(uniqid()) . '-' . date('Y');

// Create upgrade record
$upgradeId = Database::insert('device_slots_upgrades', [
    'license_id' => $licenseId,
    'user_id' => current_user_id(),
    'additional_slots' => $pricing['slots_count'],
    'price' => $pricing['price'],
    'order_number' => $orderNumber,
    'status' => 'pending'
]);

// For now, we'll simulate payment completion (in real implementation, integrate with payment gateway)
// In production, you'd redirect to PayPal/Stripe and handle webhook callback

// Simulate successful payment
Database::update('device_slots_upgrades', [
    'status' => 'completed',
    'payment_method' => 'test_payment',
    'transaction_id' => 'TEST_' . $orderNumber,
    'paid_at' => date('Y-m-d H:i:s'),
    'updated_at' => date('Y-m-d H:i:s')
], 'id = ?', [$upgradeId]);

// Update license with additional slots
$newMaxSlots = $license['max_activations'] + $pricing['slots_count'];

// Update upgrade history
$history = $license['upgrade_history'] ? json_decode($license['upgrade_history'], true) : [];
$history[] = [
    'date' => date('Y-m-d H:i:s'),
    'slots' => $pricing['slots_count'],
    'price' => $pricing['price'],
    'previous_max' => $license['max_activations'],
    'new_max' => $newMaxSlots
];

// Update license
Database::update('licenses', [
    'max_activations_updated' => $newMaxSlots,
    'upgrade_history' => json_encode($history),
    'updated_at' => date('Y-m-d H:i:s')
], 'id = ?', [$licenseId]);

set_flash('success', "Device slots upgraded successfully! You now have {$newMaxSlots} total slots for {$license['plugin_name']}.");
redirect('/dashboard');
