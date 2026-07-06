<?php
$pageTitle = 'Manage Users - ' . SITE_NAME;
$page = max(1, (int)($_GET['page'] ?? 1));
$total = Database::count('users');
$pagination = paginate($total, ADMIN_ITEMS_PER_PAGE, $page);

$users = Database::fetchAll(
    "SELECT u.*, 
        (SELECT COUNT(*) FROM orders WHERE user_id = u.id) as order_count,
        (SELECT COUNT(*) FROM licenses WHERE user_id = u.id) as license_count
     FROM users u ORDER BY u.created_at DESC LIMIT ? OFFSET ?",
    [$pagination['per_page'], $pagination['offset']]
);

require BASE_PATH . '/includes/admin-header.php';
?>

<div class="mb-6">
    <h1 class="text-2xl font-bold text-white">Users</h1>
    <p class="text-gray-400 text-sm mt-1"><?= $total ?> registered users</p>
</div>

<div class="bg-gray-900 border border-gray-800 rounded-2xl overflow-hidden">
    <div class="overflow-x-auto">
        <table class="w-full text-sm text-left">
            <thead class="bg-gray-800/50 text-xs text-gray-400 uppercase tracking-wider">
                <tr>
                    <th class="px-6 py-3">User</th>
                    <th class="px-6 py-3">Role</th>
                    <th class="px-6 py-3">Orders</th>
                    <th class="px-6 py-3">Licenses</th>
                    <th class="px-6 py-3">Status</th>
                    <th class="px-6 py-3">Joined</th>
                    <th class="px-6 py-3">Actions</th>
                </tr>
            </thead>
            <tbody class="divide-y divide-gray-800">
                <?php foreach ($users as $u): ?>
                <tr class="hover:bg-gray-800/30 transition">
                    <td class="px-6 py-4">
                        <div class="flex items-center gap-3">
                            <div class="w-8 h-8 bg-gradient-to-br from-brand-500 to-purple-600 rounded-full flex items-center justify-center text-xs font-bold text-white"><?= strtoupper(substr($u['name'], 0, 1)) ?></div>
                            <div>
                                <div class="font-medium text-white"><?= e($u['name']) ?></div>
                                <div class="text-xs text-gray-500"><?= e($u['email']) ?></div>
                            </div>
                        </div>
                    </td>
                    <td class="px-6 py-4">
                        <span class="px-2 py-0.5 rounded text-xs font-medium <?= $u['role'] === 'admin' ? 'bg-brand-500/10 text-brand-400' : 'bg-gray-500/10 text-gray-400' ?>"><?= ucfirst($u['role']) ?></span>
                    </td>
                    <td class="px-6 py-4 text-gray-400"><?= $u['order_count'] ?></td>
                    <td class="px-6 py-4 text-gray-400"><?= $u['license_count'] ?></td>
                    <td class="px-6 py-4">
                        <?php if ($u['is_active']): ?>
                            <span class="inline-flex items-center px-2 py-0.5 rounded-full text-xs font-medium bg-emerald-500/10 text-emerald-400">Active</span>
                        <?php else: ?>
                            <span class="inline-flex items-center px-2 py-0.5 rounded-full text-xs font-medium bg-red-500/10 text-red-400">Disabled</span>
                        <?php endif; ?>
                    </td>
                    <td class="px-6 py-4 text-gray-400"><?= format_date($u['created_at']) ?></td>
                    <td class="px-6 py-4">
                        <div class="flex items-center gap-2">
                            <a href="/admin/users/<?= $u['id'] ?>" class="text-brand-400 hover:text-brand-300 text-xs font-medium">View</a>
                            <a href="/admin/users/edit/<?= $u['id'] ?>" class="text-gray-400 hover:text-white text-xs font-medium">Edit</a>
                        </div>
                    </td>
                </tr>
                <?php endforeach; ?>
                <?php if (empty($users)): ?>
                <tr><td colspan="7" class="px-6 py-12 text-center text-gray-500">No users found.</td></tr>
                <?php endif; ?>
            </tbody>
        </table>
    </div>
</div>
<?= render_pagination($pagination, '/admin/users') ?>

<?php require BASE_PATH . '/includes/admin-footer.php'; ?>
