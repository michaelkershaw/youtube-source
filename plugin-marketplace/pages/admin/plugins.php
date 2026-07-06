<?php
$pageTitle = 'Manage Plugins - ' . SITE_NAME;
$page = max(1, (int)($_GET['page'] ?? 1));
$total = Database::count('plugins');
$pagination = paginate($total, ADMIN_ITEMS_PER_PAGE, $page);

$plugins = Database::fetchAll(
    "SELECT p.*, 
        (SELECT COUNT(*) FROM orders WHERE plugin_id = p.id) as order_count,
        (SELECT COUNT(*) FROM licenses WHERE plugin_id = p.id) as license_count,
        (SELECT version FROM plugin_versions WHERE plugin_id = p.id AND is_latest = 1 LIMIT 1) as latest_version
     FROM plugins p ORDER BY p.created_at DESC LIMIT ? OFFSET ?",
    [$pagination['per_page'], $pagination['offset']]
);

require BASE_PATH . '/includes/admin-header.php';
?>

<div class="flex items-center justify-between mb-6">
    <div>
        <h1 class="text-2xl font-bold text-white">Plugins</h1>
        <p class="text-gray-400 text-sm mt-1"><?= $total ?> total plugins</p>
    </div>
    <a href="/admin/plugins/create" class="px-4 py-2.5 bg-brand-600 hover:bg-brand-500 text-white text-sm font-medium rounded-xl transition flex items-center gap-2">
        <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 6v6m0 0v6m0-6h6m-6 0H6"/></svg>
        Add Plugin
    </a>
</div>

<div class="bg-gray-900 border border-gray-800 rounded-2xl overflow-hidden">
    <div class="overflow-x-auto">
        <table class="w-full text-sm text-left">
            <thead class="bg-gray-800/50 text-xs text-gray-400 uppercase tracking-wider">
                <tr>
                    <th class="px-6 py-3">Plugin</th>
                    <th class="px-6 py-3">Category</th>
                    <th class="px-6 py-3">Version</th>
                    <th class="px-6 py-3">Sales</th>
                    <th class="px-6 py-3">Downloads</th>
                    <th class="px-6 py-3">Licenses</th>
                    <th class="px-6 py-3">Status</th>
                    <th class="px-6 py-3">Actions</th>
                </tr>
            </thead>
            <tbody class="divide-y divide-gray-800">
                <?php foreach ($plugins as $p): ?>
                <tr class="hover:bg-gray-800/30 transition">
                    <td class="px-6 py-4">
                        <div class="flex items-center gap-3">
                            <div class="font-medium text-white"><?= e($p['name']) ?></div>
                            <?php if ($p['is_featured']): ?>
                                <span class="px-1.5 py-0.5 bg-amber-500/10 text-amber-400 text-xs rounded">Featured</span>
                            <?php endif; ?>
                        </div>
                        <div class="text-xs text-gray-500 mt-0.5">/plugin/<?= e($p['slug']) ?></div>
                    </td>
                    <td class="px-6 py-4 text-gray-400"><?= e($p['category'] ?: '—') ?></td>
                    <td class="px-6 py-4 text-gray-400"><?= $p['latest_version'] ? 'v' . e($p['latest_version']) : '—' ?></td>
                    <td class="px-6 py-4 text-gray-400"><?= $p['sales_count'] ?></td>
                    <td class="px-6 py-4 text-gray-400"><?= $p['downloads_count'] ?></td>
                    <td class="px-6 py-4 text-gray-400"><?= $p['license_count'] ?></td>
                    <td class="px-6 py-4">
                        <?php if ($p['is_active']): ?>
                            <span class="inline-flex items-center px-2 py-0.5 rounded-full text-xs font-medium bg-emerald-500/10 text-emerald-400">Active</span>
                        <?php else: ?>
                            <span class="inline-flex items-center px-2 py-0.5 rounded-full text-xs font-medium bg-gray-500/10 text-gray-400">Inactive</span>
                        <?php endif; ?>
                    </td>
                    <td class="px-6 py-4">
                        <div class="flex items-center gap-2">
                            <a href="/admin/plugins/<?= $p['id'] ?>" class="text-brand-400 hover:text-brand-300 text-xs font-medium">View</a>
                            <a href="/admin/plugins/edit/<?= $p['id'] ?>" class="text-gray-400 hover:text-white text-xs font-medium">Edit</a>
                            <a href="/admin/plugins/delete/<?= $p['id'] ?>" class="text-red-400 hover:text-red-300 text-xs font-medium" onclick="return confirm('Delete this plugin?')">Delete</a>
                        </div>
                    </td>
                </tr>
                <?php endforeach; ?>
                <?php if (empty($plugins)): ?>
                <tr><td colspan="8" class="px-6 py-12 text-center text-gray-500">No plugins yet. <a href="/admin/plugins/create" class="text-brand-400">Create one</a></td></tr>
                <?php endif; ?>
            </tbody>
        </table>
    </div>
</div>
<?= render_pagination($pagination, '/admin/plugins') ?>

<?php require BASE_PATH . '/includes/admin-footer.php'; ?>
