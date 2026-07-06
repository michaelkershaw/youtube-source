<?php
require_login();

$pluginId = (int)($_GET['plugin_id'] ?? 0);
$pricingId = (int)($_GET['pricing_id'] ?? 0);

$plugin = Database::fetch("SELECT * FROM plugins WHERE id = ? AND is_active = 1", [$pluginId]);
$pricing = Database::fetch("SELECT * FROM plugin_pricings WHERE id = ? AND plugin_id = ? AND is_active = 1", [$pricingId, $pluginId]);

if (!$plugin || !$pricing) {
    set_flash('error', 'Invalid plugin or pricing option.');
    redirect('/plugins');
}

$pageTitle = 'Checkout - ' . e($plugin['name']) . ' - ' . SITE_NAME;
$user = current_user();

// Check if user already owns active license
$existingLicense = Database::fetch(
    "SELECT * FROM licenses WHERE user_id = ? AND plugin_id = ? AND status = 'active' AND (expires_at IS NULL OR expires_at > NOW())",
    [$user['id'], $plugin['id']]
);

if ($existingLicense) {
    set_flash('warning', 'You already have an active license for this plugin.');
    redirect('/dashboard');
}

if ($method === 'POST') {
    require_csrf();

    // Mock payment processing (replace with Stripe/PayPal in production)
    $transactionId = 'TXN-' . strtoupper(bin2hex(random_bytes(8)));

    // Create order
    $orderId = Database::insert('orders', [
        'user_id' => $user['id'],
        'plugin_id' => $plugin['id'],
        'plugin_pricing_id' => $pricing['id'],
        'order_number' => generate_order_number(),
        'license_type' => $pricing['license_type'],
        'custom_days' => $pricing['custom_days'],
        'price' => $pricing['price'],
        'status' => 'completed',
        'payment_method' => 'mock',
        'transaction_id' => $transactionId,
        'paid_at' => date('Y-m-d H:i:s'),
        'created_at' => date('Y-m-d H:i:s'),
        'updated_at' => date('Y-m-d H:i:s'),
    ]);

    // Generate license key
    $licenseKey = LicenseKeyGenerator::generate($plugin['id']);
    $expiresAt = LicenseKeyGenerator::calculateExpiration($pricing['license_type'], $pricing['custom_days']);

    // Create license
    Database::insert('licenses', [
        'user_id' => $user['id'],
        'plugin_id' => $plugin['id'],
        'order_id' => $orderId,
        'license_key' => $licenseKey,
        'license_type' => $pricing['license_type'],
        'custom_days' => $pricing['custom_days'],
        'status' => 'active',
        'starts_at' => date('Y-m-d H:i:s'),
        'expires_at' => $expiresAt,
        'max_activations' => DEFAULT_MAX_ACTIVATIONS,
        'current_activations' => 0,
        'created_at' => date('Y-m-d H:i:s'),
        'updated_at' => date('Y-m-d H:i:s'),
    ]);

    // Increment sales count
    Database::query("UPDATE plugins SET sales_count = sales_count + 1 WHERE id = ?", [$plugin['id']]);

    set_flash('success', 'Purchase completed successfully!');
    redirect('/checkout/success/' . $orderId);
}

$licenseLabel = match($pricing['license_type']) {
    'monthly' => 'Monthly License',
    'yearly' => 'Yearly License',
    'lifetime' => 'Lifetime License',
    'custom_days' => $pricing['custom_days'] . ' Days License',
    default => ucfirst($pricing['license_type']),
};

require BASE_PATH . '/includes/header.php';
?>

<div class="max-w-3xl mx-auto px-4 sm:px-6 lg:px-8 py-12">
    <nav class="flex items-center gap-2 text-sm text-gray-500 mb-8">
        <a href="/plugins" class="hover:text-gray-300 transition">Plugins</a>
        <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M9 5l7 7-7 7"/></svg>
        <a href="/plugin/<?= e($plugin['slug']) ?>" class="hover:text-gray-300 transition"><?= e($plugin['name']) ?></a>
        <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M9 5l7 7-7 7"/></svg>
        <span class="text-gray-400">Checkout</span>
    </nav>

    <h1 class="text-3xl font-bold text-white mb-8">Complete Your Purchase</h1>

    <div class="grid grid-cols-1 md:grid-cols-5 gap-8">
        <!-- Order Summary -->
        <div class="md:col-span-3">
            <div class="bg-gray-900 border border-gray-800 rounded-2xl p-6">
                <h2 class="text-lg font-semibold text-white mb-6">Order Summary</h2>
                <div class="flex items-start gap-4 pb-6 border-b border-gray-800">
                    <div class="w-16 h-16 bg-gradient-to-br from-brand-600/20 to-purple-600/20 rounded-xl flex items-center justify-center flex-shrink-0">
                        <svg class="w-8 h-8 text-brand-400" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="1.5" d="M20 7l-8-4-8 4m16 0l-8 4m8-4v10l-8 4m0-10L4 7m8 4v10M4 7v10l8 4"/></svg>
                    </div>
                    <div class="flex-1">
                        <h3 class="font-semibold text-white"><?= e($plugin['name']) ?></h3>
                        <p class="text-sm text-gray-400 mt-1"><?= e($licenseLabel) ?></p>
                        <p class="text-xs text-gray-500 mt-1">Includes <?= DEFAULT_MAX_ACTIVATIONS ?> device activations</p>
                    </div>
                    <div class="text-xl font-bold text-white"><?= format_price($pricing['price']) ?></div>
                </div>
                <div class="pt-4 space-y-2">
                    <div class="flex justify-between text-sm">
                        <span class="text-gray-400">Subtotal</span>
                        <span class="text-white"><?= format_price($pricing['price']) ?></span>
                    </div>
                    <div class="flex justify-between text-lg font-bold pt-2 border-t border-gray-800 mt-2">
                        <span class="text-white">Total</span>
                        <span class="text-brand-400"><?= format_price($pricing['price']) ?></span>
                    </div>
                </div>
            </div>
        </div>

        <!-- Payment Form -->
        <div class="md:col-span-2">
            <div class="bg-gray-900 border border-gray-800 rounded-2xl p-6">
                <h2 class="text-lg font-semibold text-white mb-4">Payment</h2>
                <div class="bg-amber-500/10 border border-amber-500/20 text-amber-400 text-xs px-4 py-3 rounded-lg mb-4">
                    <strong>Demo Mode:</strong> No real payment will be processed. Click below to simulate a purchase.
                </div>
                <form method="POST" action="/checkout/<?= $plugin['id'] ?>/<?= $pricing['id'] ?>">
                    <?= csrf_field() ?>
                    <button type="submit" class="w-full py-3.5 bg-brand-600 hover:bg-brand-500 text-white font-semibold rounded-xl transition shadow-lg shadow-brand-600/20 flex items-center justify-center gap-2">
                        <svg class="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 15v2m-6 4h12a2 2 0 002-2v-6a2 2 0 00-2-2H6a2 2 0 00-2 2v6a2 2 0 002 2zm10-10V7a4 4 0 00-8 0v4h8z"/></svg>
                        Pay <?= format_price($pricing['price']) ?>
                    </button>
                </form>
                <p class="text-xs text-gray-500 text-center mt-4">Your license key will be generated instantly after payment.</p>
            </div>
        </div>
    </div>
</div>

<?php require BASE_PATH . '/includes/footer.php'; ?>
