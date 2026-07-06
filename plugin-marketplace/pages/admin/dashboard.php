<?php
$pageTitle = 'Admin Dashboard - ' . SITE_NAME;

$totalPlugins = Database::count('plugins');
$totalUsers = Database::count('users');
$totalOrders = Database::count('orders');
$totalLicenses = Database::count('licenses');
$totalRevenue = Database::fetch("SELECT COALESCE(SUM(price), 0) as total FROM orders WHERE status = 'completed'")['total'];
$activeLicenses = Database::count('licenses', "status = 'active' AND (expires_at IS NULL OR expires_at > NOW())");

// Recent orders
$recentOrders = Database::fetchAll(
    "SELECT o.*, u.name as user_name, p.name as plugin_name 
     FROM orders o JOIN users u ON o.user_id = u.id JOIN plugins p ON o.plugin_id = p.id 
     ORDER BY o.created_at DESC LIMIT 5"
);

// Recent users
$recentUsers = Database::fetchAll("SELECT * FROM users ORDER BY created_at DESC LIMIT 5");

require BASE_PATH . '/includes/admin-header.php';
?>

<div class="mb-8">
    <h1 class="text-2xl font-bold text-white">Dashboard</h1>
    <p class="text-gray-400 mt-1">Overview of your marketplace</p>
</div>

<!-- Stats Grid -->
<div class="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-6 gap-4 mb-8">
    <div class="bg-gray-900 border border-gray-800 rounded-xl p-5">
        <div class="text-xs text-gray-500 uppercase tracking-wider mb-1">Plugins</div>
        <div class="text-2xl font-bold text-white"><?= $totalPlugins ?></div>
    </div>
    <div class="bg-gray-900 border border-gray-800 rounded-xl p-5">
        <div class="text-xs text-gray-500 uppercase tracking-wider mb-1">Users</div>
        <div class="text-2xl font-bold text-white"><?= $totalUsers ?></div>
    </div>
    <div class="bg-gray-900 border border-gray-800 rounded-xl p-5">
        <div class="text-xs text-gray-500 uppercase tracking-wider mb-1">Orders</div>
        <div class="text-2xl font-bold text-white"><?= $totalOrders ?></div>
    </div>
    <div class="bg-gray-900 border border-gray-800 rounded-xl p-5">
        <div class="text-xs text-gray-500 uppercase tracking-wider mb-1">Revenue</div>
        <div class="text-2xl font-bold text-emerald-400"><?= format_price($totalRevenue) ?></div>
    </div>
    <div class="bg-gray-900 border border-gray-800 rounded-xl p-5">
        <div class="text-xs text-gray-500 uppercase tracking-wider mb-1">Licenses</div>
        <div class="text-2xl font-bold text-white"><?= $totalLicenses ?></div>
    </div>
    <div class="bg-gray-900 border border-gray-800 rounded-xl p-5">
        <div class="text-xs text-gray-500 uppercase tracking-wider mb-1">Active</div>
        <div class="text-2xl font-bold text-brand-400"><?= $activeLicenses ?></div>
    </div>
</div>

<div class="grid grid-cols-1 lg:grid-cols-2 gap-6">
    <!-- Recent Orders -->
    <div class="bg-gray-900 border border-gray-800 rounded-2xl overflow-hidden">
        <div class="px-6 py-4 border-b border-gray-800 flex items-center justify-between">
            <h2 class="font-semibold text-white">Recent Orders</h2>
            <a href="/admin/orders" class="text-brand-400 text-sm hover:text-brand-300">View All</a>
        </div>
        <?php if (empty($recentOrders)): ?>
            <div class="px-6 py-8 text-center text-gray-500 text-sm">No orders yet.</div>
        <?php else: ?>
        <div class="divide-y divide-gray-800">
            <?php foreach ($recentOrders as $o): ?>
            <div class="px-6 py-3 flex items-center justify-between">
                <div>
                    <a href="/admin/orders/<?= $o['id'] ?>" class="text-sm font-medium text-white hover:text-brand-400 transition"><?= e($o['order_number']) ?></a>
                    <div class="text-xs text-gray-500"><?= e($o['user_name']) ?> &middot; <?= e($o['plugin_name']) ?></div>
                </div>
                <div class="text-right">
                    <div class="text-sm font-medium text-white"><?= format_price($o['price']) ?></div>
                    <div class="text-xs text-gray-500"><?= time_ago($o['created_at']) ?></div>
                </div>
            </div>
            <?php endforeach; ?>
        </div>
        <?php endif; ?>
    </div>

    <!-- Recent Users -->
    <div class="bg-gray-900 border border-gray-800 rounded-2xl overflow-hidden">
        <div class="px-6 py-4 border-b border-gray-800 flex items-center justify-between">
            <h2 class="font-semibold text-white">Recent Users</h2>
            <a href="/admin/users" class="text-brand-400 text-sm hover:text-brand-300">View All</a>
        </div>
        <?php if (empty($recentUsers)): ?>
            <div class="px-6 py-8 text-center text-gray-500 text-sm">No users yet.</div>
        <?php else: ?>
        <div class="divide-y divide-gray-800">
            <?php foreach ($recentUsers as $u): ?>
            <div class="px-6 py-3 flex items-center justify-between">
                <div class="flex items-center gap-3">
                    <div class="w-8 h-8 bg-gradient-to-br from-brand-500 to-purple-600 rounded-full flex items-center justify-center text-xs font-bold text-white">
                        <?= strtoupper(substr($u['name'], 0, 1)) ?>
                    </div>
                    <div>
                        <a href="/admin/users/<?= $u['id'] ?>" class="text-sm font-medium text-white hover:text-brand-400 transition"><?= e($u['name']) ?></a>
                        <div class="text-xs text-gray-500"><?= e($u['email']) ?></div>
                    </div>
                </div>
                <div class="text-xs text-gray-500"><?= time_ago($u['created_at']) ?></div>
            </div>
            <?php endforeach; ?>
        </div>
        <?php endif; ?>
    </div>
</div>

<?php require BASE_PATH . '/includes/admin-footer.php'; ?>
