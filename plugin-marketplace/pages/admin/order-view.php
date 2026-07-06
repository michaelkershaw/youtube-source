<?php
$pageTitle = 'View Order - ' . SITE_NAME;
$id = (int)($_GET['id'] ?? 0);
$order = Database::fetch(
    "SELECT o.*, u.name as user_name, u.email as user_email, p.name as plugin_name, p.slug as plugin_slug,
        pp.license_type as pricing_type, pp.price as pricing_price
     FROM orders o
     JOIN users u ON o.user_id = u.id
     JOIN plugins p ON o.plugin_id = p.id
     JOIN plugin_pricings pp ON o.plugin_pricing_id = pp.id
     WHERE o.id = ?",
    [$id]
);
if (!$order) { set_flash('error', 'Order not found.'); redirect('/admin/orders'); }

$license = Database::fetch("SELECT * FROM licenses WHERE order_id = ?", [$id]);

require BASE_PATH . '/includes/admin-header.php';
?>

<div class="mb-6">
    <a href="/admin/orders" class="text-sm text-gray-400 hover:text-white transition flex items-center gap-1 mb-2">
        <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M10 19l-7-7m0 0l7-7m-7 7h18"/></svg> Back to Orders
    </a>
    <div class="flex items-center justify-between">
        <h1 class="text-2xl font-bold text-white">Order: <?= e($order['order_number']) ?></h1>
        <?php if ($order['status'] === 'completed'): ?>
        <form method="POST" action="/admin/orders/refund/<?= $id ?>" onsubmit="return confirm('Are you sure you want to refund this order? The associated license will be revoked.')">
            <?= csrf_field() ?>
            <button type="submit" class="px-4 py-2 bg-red-600 hover:bg-red-500 text-white text-sm font-medium rounded-xl transition">Issue Refund</button>
        </form>
        <?php endif; ?>
    </div>
</div>

<div class="grid grid-cols-1 lg:grid-cols-2 gap-6">
    <!-- Order Details -->
    <div class="bg-gray-900 border border-gray-800 rounded-2xl p-6">
        <h2 class="font-semibold text-white mb-4">Order Details</h2>
        <dl class="space-y-3 text-sm">
            <div class="flex justify-between"><dt class="text-gray-400">Order Number</dt><dd class="text-white font-mono"><?= e($order['order_number']) ?></dd></div>
            <div class="flex justify-between"><dt class="text-gray-400">Status</dt><dd>
                <?php
                $sc = match($order['status']) {
                    'completed' => 'bg-emerald-500/10 text-emerald-400 border-emerald-500/20',
                    'pending' => 'bg-amber-500/10 text-amber-400 border-amber-500/20',
                    'refunded' => 'bg-red-500/10 text-red-400 border-red-500/20',
                    default => 'bg-gray-500/10 text-gray-400 border-gray-500/20',
                };
                ?>
                <span class="px-2.5 py-0.5 rounded-full text-xs font-medium <?= $sc ?> border"><?= ucfirst($order['status']) ?></span>
            </dd></div>
            <div class="flex justify-between"><dt class="text-gray-400">Amount</dt><dd class="text-white font-bold text-lg"><?= format_price($order['price']) ?></dd></div>
            <div class="flex justify-between"><dt class="text-gray-400">License Type</dt><dd class="text-white capitalize"><?= e(str_replace('_', ' ', $order['license_type'])) ?></dd></div>
            <div class="flex justify-between"><dt class="text-gray-400">Payment Method</dt><dd class="text-white"><?= e($order['payment_method'] ?? '—') ?></dd></div>
            <div class="flex justify-between"><dt class="text-gray-400">Transaction ID</dt><dd class="text-gray-300 font-mono text-xs"><?= e($order['transaction_id'] ?? '—') ?></dd></div>
            <div class="flex justify-between"><dt class="text-gray-400">Paid At</dt><dd class="text-white"><?= $order['paid_at'] ? format_datetime($order['paid_at']) : '—' ?></dd></div>
            <?php if ($order['refunded_at']): ?>
            <div class="flex justify-between"><dt class="text-gray-400">Refunded At</dt><dd class="text-red-400"><?= format_datetime($order['refunded_at']) ?></dd></div>
            <?php endif; ?>
            <div class="flex justify-between"><dt class="text-gray-400">Created</dt><dd class="text-white"><?= format_datetime($order['created_at']) ?></dd></div>
            <?php if ($order['notes']): ?>
            <div class="pt-2 border-t border-gray-800"><dt class="text-gray-400 mb-1">Notes</dt><dd class="text-gray-300 text-xs"><?= nl2br(e($order['notes'])) ?></dd></div>
            <?php endif; ?>
        </dl>
    </div>

    <!-- Customer & License -->
    <div class="space-y-6">
        <div class="bg-gray-900 border border-gray-800 rounded-2xl p-6">
            <h2 class="font-semibold text-white mb-4">Customer</h2>
            <div class="flex items-center gap-3 mb-3">
                <div class="w-10 h-10 bg-gradient-to-br from-brand-500 to-purple-600 rounded-full flex items-center justify-center text-sm font-bold text-white"><?= strtoupper(substr($order['user_name'], 0, 1)) ?></div>
                <div>
                    <a href="/admin/users/<?= $order['user_id'] ?>" class="text-white font-medium hover:text-brand-400 transition"><?= e($order['user_name']) ?></a>
                    <div class="text-xs text-gray-500"><?= e($order['user_email']) ?></div>
                </div>
            </div>
        </div>

        <div class="bg-gray-900 border border-gray-800 rounded-2xl p-6">
            <h2 class="font-semibold text-white mb-4">Plugin</h2>
            <a href="/admin/plugins/<?= $order['plugin_id'] ?>" class="text-brand-400 hover:text-brand-300 font-medium transition"><?= e($order['plugin_name']) ?></a>
        </div>

        <?php if ($license): ?>
        <div class="bg-gray-900 border border-gray-800 rounded-2xl p-6">
            <h2 class="font-semibold text-white mb-4">License</h2>
            <dl class="space-y-2 text-sm">
                <div class="flex justify-between"><dt class="text-gray-400">Key</dt><dd class="font-mono text-xs text-brand-400"><?= e($license['license_key']) ?></dd></div>
                <div class="flex justify-between"><dt class="text-gray-400">Status</dt><dd>
                    <span class="px-2 py-0.5 rounded-full text-xs <?= $license['status'] === 'active' ? 'bg-emerald-500/10 text-emerald-400' : ($license['status'] === 'expired' ? 'bg-amber-500/10 text-amber-400' : 'bg-red-500/10 text-red-400') ?>"><?= ucfirst($license['status']) ?></span>
                </dd></div>
                <div class="flex justify-between"><dt class="text-gray-400">Expires</dt><dd class="text-white"><?= $license['expires_at'] ? format_date($license['expires_at']) : 'Lifetime' ?></dd></div>
                <div class="flex justify-between"><dt class="text-gray-400">Devices</dt><dd class="text-white"><?= $license['current_activations'] ?>/<?= $license['max_activations'] ?></dd></div>
            </dl>
            <a href="/admin/licenses/<?= $license['id'] ?>" class="mt-3 inline-block text-brand-400 hover:text-brand-300 text-xs font-medium">Manage License &rarr;</a>
        </div>
        <?php endif; ?>
    </div>
</div>

<?php require BASE_PATH . '/includes/admin-footer.php'; ?>
