<?php
$pageTitle = 'View User - ' . SITE_NAME;
$id = (int)($_GET['id'] ?? 0);
$user = Database::fetch("SELECT * FROM users WHERE id = ?", [$id]);
if (!$user) { set_flash('error', 'User not found.'); redirect('/admin/users'); }

$orders = Database::fetchAll(
    "SELECT o.*, p.name as plugin_name FROM orders o JOIN plugins p ON o.plugin_id = p.id WHERE o.user_id = ? ORDER BY o.created_at DESC",
    [$id]
);
$licenses = Database::fetchAll(
    "SELECT l.*, p.name as plugin_name FROM licenses l JOIN plugins p ON l.plugin_id = p.id WHERE l.user_id = ? ORDER BY l.created_at DESC",
    [$id]
);

require BASE_PATH . '/includes/admin-header.php';
?>

<div class="mb-6">
    <a href="/admin/users" class="text-sm text-gray-400 hover:text-white transition flex items-center gap-1 mb-2">
        <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M10 19l-7-7m0 0l7-7m-7 7h18"/></svg> Back to Users
    </a>
    <div class="flex items-center gap-4">
        <div class="w-12 h-12 bg-gradient-to-br from-brand-500 to-purple-600 rounded-full flex items-center justify-center text-lg font-bold text-white"><?= strtoupper(substr($user['name'], 0, 1)) ?></div>
        <div>
            <h1 class="text-2xl font-bold text-white"><?= e($user['name']) ?></h1>
            <p class="text-gray-400 text-sm"><?= e($user['email']) ?> &middot; <?= ucfirst($user['role']) ?> &middot; Joined <?= format_date($user['created_at']) ?></p>
        </div>
        <a href="/admin/users/edit/<?= $id ?>" class="ml-auto px-4 py-2 bg-brand-600 hover:bg-brand-500 text-white text-sm font-medium rounded-xl transition">Edit User</a>
    </div>
</div>

<!-- User Licenses -->
<div class="bg-gray-900 border border-gray-800 rounded-2xl overflow-hidden mb-6">
    <div class="px-6 py-4 border-b border-gray-800"><h2 class="font-semibold text-white">Licenses (<?= count($licenses) ?>)</h2></div>
    <?php if (empty($licenses)): ?>
        <div class="px-6 py-8 text-center text-gray-500 text-sm">No licenses.</div>
    <?php else: ?>
    <div class="overflow-x-auto">
        <table class="w-full text-sm">
            <thead class="bg-gray-800/50 text-xs text-gray-400 uppercase"><tr><th class="px-6 py-3 text-left">Plugin</th><th class="px-6 py-3 text-left">Key</th><th class="px-6 py-3 text-left">Type</th><th class="px-6 py-3 text-left">Status</th><th class="px-6 py-3 text-left">Expires</th><th class="px-6 py-3 text-left">Devices</th><th class="px-6 py-3 text-left">Action</th></tr></thead>
            <tbody class="divide-y divide-gray-800">
                <?php foreach ($licenses as $l): ?>
                <tr class="hover:bg-gray-800/30">
                    <td class="px-6 py-3 text-white"><?= e($l['plugin_name']) ?></td>
                    <td class="px-6 py-3 font-mono text-xs text-gray-300"><?= e($l['license_key']) ?></td>
                    <td class="px-6 py-3 text-gray-400 capitalize"><?= e(str_replace('_', ' ', $l['license_type'])) ?></td>
                    <td class="px-6 py-3"><span class="px-2 py-0.5 rounded-full text-xs <?= $l['status'] === 'active' ? 'bg-emerald-500/10 text-emerald-400' : ($l['status'] === 'expired' ? 'bg-amber-500/10 text-amber-400' : 'bg-red-500/10 text-red-400') ?>"><?= ucfirst($l['status']) ?></span></td>
                    <td class="px-6 py-3 text-gray-400"><?= $l['expires_at'] ? format_date($l['expires_at']) : 'Lifetime' ?></td>
                    <td class="px-6 py-3 text-gray-400"><?= $l['current_activations'] ?>/<?= $l['max_activations'] ?></td>
                    <td class="px-6 py-3"><a href="/admin/licenses/<?= $l['id'] ?>" class="text-brand-400 hover:text-brand-300 text-xs">Manage</a></td>
                </tr>
                <?php endforeach; ?>
            </tbody>
        </table>
    </div>
    <?php endif; ?>
</div>

<!-- User Orders -->
<div class="bg-gray-900 border border-gray-800 rounded-2xl overflow-hidden">
    <div class="px-6 py-4 border-b border-gray-800"><h2 class="font-semibold text-white">Orders (<?= count($orders) ?>)</h2></div>
    <?php if (empty($orders)): ?>
        <div class="px-6 py-8 text-center text-gray-500 text-sm">No orders.</div>
    <?php else: ?>
    <div class="overflow-x-auto">
        <table class="w-full text-sm">
            <thead class="bg-gray-800/50 text-xs text-gray-400 uppercase"><tr><th class="px-6 py-3 text-left">Order #</th><th class="px-6 py-3 text-left">Plugin</th><th class="px-6 py-3 text-left">Amount</th><th class="px-6 py-3 text-left">Status</th><th class="px-6 py-3 text-left">Date</th><th class="px-6 py-3 text-left">Action</th></tr></thead>
            <tbody class="divide-y divide-gray-800">
                <?php foreach ($orders as $o): ?>
                <tr class="hover:bg-gray-800/30">
                    <td class="px-6 py-3 font-mono text-xs text-white"><?= e($o['order_number']) ?></td>
                    <td class="px-6 py-3 text-gray-300"><?= e($o['plugin_name']) ?></td>
                    <td class="px-6 py-3 text-white font-medium"><?= format_price($o['price']) ?></td>
                    <td class="px-6 py-3">
                        <?php $sc = ['completed'=>'bg-emerald-500/10 text-emerald-400','pending'=>'bg-amber-500/10 text-amber-400','refunded'=>'bg-red-500/10 text-red-400','failed'=>'bg-gray-500/10 text-gray-400'][$o['status']] ?? 'bg-gray-500/10 text-gray-400'; ?>
                        <span class="px-2 py-0.5 rounded-full text-xs <?= $sc ?>"><?= ucfirst($o['status']) ?></span>
                    </td>
                    <td class="px-6 py-3 text-gray-400"><?= format_date($o['created_at']) ?></td>
                    <td class="px-6 py-3"><a href="/admin/orders/<?= $o['id'] ?>" class="text-brand-400 hover:text-brand-300 text-xs">View</a></td>
                </tr>
                <?php endforeach; ?>
            </tbody>
        </table>
    </div>
    <?php endif; ?>
</div>

<?php require BASE_PATH . '/includes/admin-footer.php'; ?>
