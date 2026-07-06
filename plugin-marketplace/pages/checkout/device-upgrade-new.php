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

require BASE_PATH . '/includes/header.php';
?>

<div class="max-w-4xl mx-auto px-4 sm:px-6 lg:px-8 py-12">
    <div class="text-center">
        <div class="mb-8">
            <div class="w-16 h-16 bg-emerald-500 rounded-full flex items-center justify-center mx-auto mb-4">
                <svg class="w-8 h-8 text-white" fill="currentColor" viewBox="0 0 20 20"><path fill-rule="evenodd" d="M16.707 5.293a1 1 0 010 1.414l-8 8a1 1 0 01-1.414 0l-4-4a1 1 0 011.414-1.414L8 12.586l7.293-7.293a1 1 0 011.414 0z" clip-rule="evenodd"/></svg>
            </div>
            <h1 class="text-3xl font-bold text-white mb-4">Upgrade Complete!</h1>
            <p class="text-gray-400 text-lg">Your device slots have been successfully upgraded.</p>
        </div>
        
        <div class="bg-gray-900 border border-gray-800 rounded-2xl p-6 mb-8">
            <h2 class="text-xl font-semibold text-white mb-4">Upgrade Details</h2>
            <div class="space-y-3 text-left max-w-md mx-auto">
                <div class="flex justify-between">
                    <span class="text-gray-400">Plugin:</span>
                    <span class="text-white"><?= e($license['plugin_name']) ?></span>
                </div>
                <div class="flex justify-between">
                    <span class="text-gray-400">Additional Slots:</span>
                    <span class="text-emerald-400 font-semibold">+<?= $pricing['slots_count'] ?></span>
                </div>
                <div class="flex justify-between">
                    <span class="text-gray-400">New Total Slots:</span>
                    <span class="text-brand-400 font-semibold"><?= $newMaxSlots ?></span>
                </div>
                <div class="flex justify-between">
                    <span class="text-gray-400">Order Number:</span>
                    <span class="text-gray-300 font-mono text-sm"><?= $orderNumber ?></span>
                </div>
            </div>
        </div>
        
        <a href="/dashboard" class="inline-flex items-center gap-2 px-6 py-3 bg-brand-600 hover:bg-brand-500 text-white font-semibold rounded-xl transition">
            <svg class="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M10 6H6a2 2 0 00-2 2v10a2 2 0 002 2h10a2 2 0 002-2v-4M14 4h6m0 0v6m0-6L10 14"/></svg>
            Return to Dashboard
        </a>
    </div>
</div>

<?php require BASE_PATH . '/includes/footer.php'; ?>
