<?php
$pageTitle = 'Manage Licenses - ' . SITE_NAME;
$page = max(1, (int)($_GET['page'] ?? 1));
$total = Database::count('licenses');
$pagination = paginate($total, ADMIN_ITEMS_PER_PAGE, $page);

$licenses = Database::fetchAll(
    "SELECT l.*, u.name as user_name, u.email as user_email, p.name as plugin_name, p.slug as plugin_slug
     FROM licenses l
     JOIN users u ON l.user_id = u.id
     LEFT JOIN plugins p ON l.plugin_id = p.id
     ORDER BY l.created_at DESC LIMIT ? OFFSET ?",
    [$pagination['per_page'], $pagination['offset']]
);

require BASE_PATH . '/includes/admin-header.php';
?>

<div class="flex items-center justify-between mb-6">
    <div>
        <h1 class="text-2xl font-bold text-white">Licenses</h1>
        <p class="text-gray-400 text-sm mt-1"><?= $total ?> total licenses</p>
    </div>
    <a href="/admin/licenses/create" class="px-4 py-2.5 bg-brand-600 hover:bg-brand-500 text-white text-sm font-medium rounded-xl transition flex items-center gap-2">
        <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 6v6m0 0v6m0-6h6m-6 0H6"/></svg>
        Generate License
    </a>
</div>

<div class="bg-gray-900 border border-gray-800 rounded-2xl overflow-hidden">
    <div class="overflow-x-auto">
        <table class="w-full text-sm text-left">
            <thead class="bg-gray-800/50 text-xs text-gray-400 uppercase tracking-wider">
                <tr>
                    <th class="px-6 py-3">License Key</th>
                    <th class="px-6 py-3">User</th>
                    <th class="px-6 py-3">Plugin</th>
                    <th class="px-6 py-3">Type</th>
                    <th class="px-6 py-3">Status</th>
                    <th class="px-6 py-3">Expires</th>
                    <th class="px-6 py-3">Devices</th>
                    <th class="px-6 py-3">Actions</th>
                </tr>
            </thead>
            <tbody class="divide-y divide-gray-800">
                <?php foreach ($licenses as $l): ?>
                <tr class="hover:bg-gray-800/30 transition">
                    <td class="px-6 py-4">
                        <code class="font-mono text-xs text-brand-400 bg-gray-800 px-2 py-0.5 rounded"><?= e($l['license_key']) ?></code>
                    </td>
                    <td class="px-6 py-4">
                        <a href="/admin/users/<?= $l['user_id'] ?>" class="text-white hover:text-brand-400 text-sm transition"><?= e($l['user_name']) ?></a>
                    </td>
                    <td class="px-6 py-4 text-gray-300"><?= e($l['plugin_name']) ?></td>
                    <td class="px-6 py-4 text-gray-400 capitalize text-xs"><?= e(str_replace('_', ' ', $l['license_type'])) ?></td>
                    <td class="px-6 py-4">
                        <?php
                        $active = LicenseKeyGenerator::isActive($l);
                        if ($active): ?>
                            <span class="px-2 py-0.5 rounded-full text-xs font-medium bg-emerald-500/10 text-emerald-400">Active</span>
                        <?php elseif ($l['status'] === 'revoked'): ?>
                            <span class="px-2 py-0.5 rounded-full text-xs font-medium bg-red-500/10 text-red-400">Revoked</span>
                        <?php else: ?>
                            <span class="px-2 py-0.5 rounded-full text-xs font-medium bg-amber-500/10 text-amber-400">Expired</span>
                        <?php endif; ?>
                    </td>
                    <td class="px-6 py-4 text-gray-400 text-xs"><?= $l['expires_at'] ? format_date($l['expires_at']) : 'Lifetime' ?></td>
                    <td class="px-6 py-4 text-gray-400"><?= $l['devices'] ?>/<?= $l['max_activations'] ?></td>
                    <td class="px-6 py-4">
                        <a href="/admin/licenses/<?= $l['id'] ?>" class="text-brand-400 hover:text-brand-300 text-xs font-medium">Manage</a>
                    </td>
                </tr>
                <?php endforeach; ?>
                <?php if (empty($licenses)): ?>
                <tr><td colspan="8" class="px-6 py-12 text-center text-gray-500">No licenses yet.</td></tr>
                <?php endif; ?>
            </tbody>
        </table>
    </div>
</div>
<?= render_pagination($pagination, '/admin/licenses') ?>

<?php require BASE_PATH . '/includes/admin-footer.php'; ?>
