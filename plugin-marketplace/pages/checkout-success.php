<?php
require_login();

$orderId = (int)($_GET['order_id'] ?? 0);
$order = Database::fetch(
    "SELECT o.*, p.name as plugin_name, p.slug as plugin_slug
     FROM orders o JOIN plugins p ON o.plugin_id = p.id
     WHERE o.id = ? AND o.user_id = ?",
    [$orderId, current_user_id()]
);

if (!$order) {
    set_flash('error', 'Order not found.');
    redirect('/dashboard');
}

$license = Database::fetch("SELECT * FROM licenses WHERE order_id = ?", [$order['id']]);
$latestVersion = Database::fetch("SELECT * FROM plugin_versions WHERE plugin_id = ? AND is_latest = 1", [$order['plugin_id']]);

$pageTitle = 'Order Confirmed - ' . SITE_NAME;
require BASE_PATH . '/includes/header.php';
?>

<div class="max-w-2xl mx-auto px-4 sm:px-6 lg:px-8 py-16">
    <div class="text-center mb-10">
        <div class="w-20 h-20 bg-emerald-500/10 rounded-full flex items-center justify-center mx-auto mb-6">
            <svg class="w-10 h-10 text-emerald-400" fill="currentColor" viewBox="0 0 20 20"><path fill-rule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zm3.707-9.293a1 1 0 00-1.414-1.414L9 10.586 7.707 9.293a1 1 0 00-1.414 1.414l2 2a1 1 0 001.414 0l4-4z" clip-rule="evenodd"/></svg>
        </div>
        <h1 class="text-3xl font-bold text-white">Purchase Complete!</h1>
        <p class="text-gray-400 mt-2">Thank you for your purchase. Your license is ready.</p>
    </div>

    <div class="bg-gray-900 border border-gray-800 rounded-2xl p-6 mb-6">
        <h2 class="text-lg font-semibold text-white mb-4">Order Details</h2>
        <dl class="space-y-3 text-sm">
            <div class="flex justify-between"><dt class="text-gray-400">Order Number</dt><dd class="text-white font-mono"><?= e($order['order_number']) ?></dd></div>
            <div class="flex justify-between"><dt class="text-gray-400">Plugin</dt><dd class="text-white"><?= e($order['plugin_name']) ?></dd></div>
            <div class="flex justify-between"><dt class="text-gray-400">License Type</dt><dd class="text-white capitalize"><?= e(str_replace('_', ' ', $order['license_type'])) ?></dd></div>
            <div class="flex justify-between"><dt class="text-gray-400">Amount Paid</dt><dd class="text-white font-semibold"><?= format_price($order['price']) ?></dd></div>
            <div class="flex justify-between"><dt class="text-gray-400">Transaction ID</dt><dd class="text-gray-300 font-mono text-xs"><?= e($order['transaction_id']) ?></dd></div>
        </dl>
    </div>

    <?php if ($license): ?>
    <div class="bg-brand-500/5 border border-brand-500/20 rounded-2xl p-6 mb-6">
        <h2 class="text-lg font-semibold text-white mb-4">Your License Key</h2>
        <div class="bg-gray-800 rounded-xl p-4 text-center mb-4">
            <code class="text-xl font-mono font-bold text-brand-400 select-all tracking-wider"><?= e($license['license_key']) ?></code>
        </div>
        <p class="text-xs text-gray-400 text-center">Copy this key and keep it safe. You can also find it in your dashboard.</p>
        <dl class="mt-4 space-y-2 text-sm">
            <div class="flex justify-between"><dt class="text-gray-400">Status</dt><dd class="text-emerald-400 font-medium">Active</dd></div>
            <div class="flex justify-between">
                <dt class="text-gray-400">Expires</dt>
                <dd class="text-white"><?= $license['expires_at'] ? format_date($license['expires_at']) : 'Never (Lifetime)' ?></dd>
            </div>
            <div class="flex justify-between"><dt class="text-gray-400">Max Devices</dt><dd class="text-white"><?= $license['max_activations'] ?></dd></div>
        </dl>
    </div>
    <?php endif; ?>

    <div class="flex flex-col sm:flex-row gap-4">
        <?php if ($latestVersion && $latestVersion['file_path']): ?>
        <a href="/download/<?= $order['plugin_id'] ?>/<?= $latestVersion['id'] ?>" class="flex-1 flex items-center justify-center gap-2 px-6 py-3 bg-emerald-600 hover:bg-emerald-500 text-white font-semibold rounded-xl transition">
            <svg class="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M4 16v1a3 3 0 003 3h10a3 3 0 003-3v-1m-4-4l-4 4m0 0l-4-4m4 4V4"/></svg>
            Download Plugin
        </a>
        <?php endif; ?>
        <a href="/dashboard" class="flex-1 flex items-center justify-center gap-2 px-6 py-3 bg-gray-800 hover:bg-gray-700 text-white font-semibold rounded-xl transition border border-gray-700">
            Go to Dashboard
        </a>
    </div>
</div>

<?php require BASE_PATH . '/includes/footer.php'; ?>
