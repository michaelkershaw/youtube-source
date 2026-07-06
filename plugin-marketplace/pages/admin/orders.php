<?php
$pageTitle = 'Manage Orders - ' . SITE_NAME;
$page = max(1, (int)($_GET['page'] ?? 1));
$total = Database::count('orders');
$pagination = paginate($total, ADMIN_ITEMS_PER_PAGE, $page);

$totalRevenue = Database::fetch("SELECT COALESCE(SUM(price), 0) as total FROM orders WHERE status = 'completed'")['total'];

$orders = Database::fetchAll(
    "SELECT o.*, u.name as user_name, u.email as user_email, p.name as plugin_name
     FROM orders o
     JOIN users u ON o.user_id = u.id
     JOIN plugins p ON o.plugin_id = p.id
     ORDER BY o.created_at DESC LIMIT ? OFFSET ?",
    [$pagination['per_page'], $pagination['offset']]
);

require BASE_PATH . '/includes/admin-header.php';
?>

<div class="flex items-center justify-between mb-6">
    <div>
        <h1 class="text-2xl font-bold text-white">Orders</h1>
        <p class="text-gray-400 text-sm mt-1"><?= $total ?> total orders &middot; Revenue: <span class="text-emerald-400 font-medium"><?= format_price($totalRevenue) ?></span></p>
    </div>
</div>

<div class="bg-gray-900 border border-gray-800 rounded-2xl overflow-hidden">
    <div class="overflow-x-auto">
        <table class="w-full text-sm text-left">
            <thead class="bg-gray-800/50 text-xs text-gray-400 uppercase tracking-wider">
                <tr>
                    <th class="px-6 py-3">Order #</th>
                    <th class="px-6 py-3">Customer</th>
                    <th class="px-6 py-3">Plugin</th>
                    <th class="px-6 py-3">License</th>
                    <th class="px-6 py-3">Amount</th>
                    <th class="px-6 py-3">Status</th>
                    <th class="px-6 py-3">Date</th>
                    <th class="px-6 py-3">Actions</th>
                </tr>
            </thead>
            <tbody class="divide-y divide-gray-800">
                <?php foreach ($orders as $o): ?>
                <tr class="hover:bg-gray-800/30 transition">
                    <td class="px-6 py-4 font-mono text-xs text-white"><?= e($o['order_number']) ?></td>
                    <td class="px-6 py-4">
                        <div class="text-white text-sm"><?= e($o['user_name']) ?></div>
                        <div class="text-xs text-gray-500"><?= e($o['user_email']) ?></div>
                    </td>
                    <td class="px-6 py-4 text-gray-300"><?= e($o['plugin_name']) ?></td>
                    <td class="px-6 py-4 text-gray-400 capitalize text-xs"><?= e(str_replace('_', ' ', $o['license_type'])) ?></td>
                    <td class="px-6 py-4 text-white font-medium"><?= format_price($o['price']) ?></td>
                    <td class="px-6 py-4">
                        <?php
                        $sc = match($o['status']) {
                            'completed' => 'bg-emerald-500/10 text-emerald-400',
                            'pending' => 'bg-amber-500/10 text-amber-400',
                            'refunded' => 'bg-red-500/10 text-red-400',
                            default => 'bg-gray-500/10 text-gray-400',
                        };
                        ?>
                        <span class="px-2 py-0.5 rounded-full text-xs font-medium <?= $sc ?>"><?= ucfirst($o['status']) ?></span>
                    </td>
                    <td class="px-6 py-4 text-gray-400 text-xs"><?= format_date($o['created_at']) ?></td>
                    <td class="px-6 py-4">
                        <a href="/admin/orders/<?= $o['id'] ?>" class="text-brand-400 hover:text-brand-300 text-xs font-medium">View</a>
                    </td>
                </tr>
                <?php endforeach; ?>
                <?php if (empty($orders)): ?>
                <tr><td colspan="8" class="px-6 py-12 text-center text-gray-500">No orders yet.</td></tr>
                <?php endif; ?>
            </tbody>
        </table>
    </div>
</div>
<?= render_pagination($pagination, '/admin/orders') ?>

<?php require BASE_PATH . '/includes/admin-footer.php'; ?>
