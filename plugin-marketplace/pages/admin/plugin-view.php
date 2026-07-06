<?php
$pageTitle = 'View Plugin - ' . SITE_NAME;
$id = (int)($_GET['id'] ?? 0);
$plugin = Database::fetch("SELECT * FROM plugins WHERE id = ?", [$id]);
if (!$plugin) { set_flash('error', 'Plugin not found.'); redirect('/admin/plugins'); }

$versions = Database::fetchAll("SELECT * FROM plugin_versions WHERE plugin_id = ? ORDER BY created_at DESC", [$id]);
$images = Database::fetchAll("SELECT * FROM plugin_images WHERE plugin_id = ? ORDER BY sort_order", [$id]);
$pricings = Database::fetchAll("SELECT * FROM plugin_pricings WHERE plugin_id = ? AND is_active = 1 ORDER BY price", [$id]);
$licenseFormat = Database::fetch("SELECT * FROM license_formats WHERE plugin_id = ?", [$id]);
$licenses = Database::fetchAll("SELECT l.*, u.name as user_name FROM licenses l JOIN users u ON l.user_id = u.id WHERE l.plugin_id = ? ORDER BY l.created_at DESC LIMIT 10", [$id]);
$orders = Database::fetchAll("SELECT o.*, u.name as user_name FROM orders o JOIN users u ON o.user_id = u.id WHERE o.plugin_id = ? ORDER BY o.created_at DESC LIMIT 10", [$id]);

require BASE_PATH . '/includes/admin-header.php';
?>

<div class="mb-6">
    <a href="/admin/plugins" class="text-sm text-gray-400 hover:text-white transition flex items-center gap-1 mb-2">
        <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M10 19l-7-7m0 0l7-7m-7 7h18"/></svg> Back to Plugins
    </a>
    <div class="flex items-center justify-between">
        <div class="flex items-center gap-3">
            <h1 class="text-2xl font-bold text-white"><?= e($plugin['name']) ?></h1>
            <?php if ($plugin['is_featured']): ?><span class="px-2 py-0.5 bg-amber-500/10 text-amber-400 text-xs font-medium rounded">Featured</span><?php endif; ?>
            <?php if ($plugin['is_active']): ?><span class="px-2 py-0.5 bg-emerald-500/10 text-emerald-400 text-xs font-medium rounded">Active</span><?php else: ?><span class="px-2 py-0.5 bg-gray-500/10 text-gray-400 text-xs font-medium rounded">Inactive</span><?php endif; ?>
        </div>
        <a href="/admin/plugins/edit/<?= $id ?>" class="px-4 py-2 bg-brand-600 hover:bg-brand-500 text-white text-sm font-medium rounded-xl transition">Edit Plugin</a>
    </div>
</div>

<!-- Stats -->
<div class="grid grid-cols-2 sm:grid-cols-4 gap-4 mb-8">
    <div class="bg-gray-900 border border-gray-800 rounded-xl p-4 text-center">
        <div class="text-xl font-bold text-white"><?= $plugin['sales_count'] ?></div>
        <div class="text-xs text-gray-500">Sales</div>
    </div>
    <div class="bg-gray-900 border border-gray-800 rounded-xl p-4 text-center">
        <div class="text-xl font-bold text-white"><?= $plugin['downloads_count'] ?></div>
        <div class="text-xs text-gray-500">Downloads</div>
    </div>
    <div class="bg-gray-900 border border-gray-800 rounded-xl p-4 text-center">
        <div class="text-xl font-bold text-white"><?= count($versions) ?></div>
        <div class="text-xs text-gray-500">Versions</div>
    </div>
    <div class="bg-gray-900 border border-gray-800 rounded-xl p-4 text-center">
        <div class="text-xl font-bold text-white"><?= count($licenses) ?></div>
        <div class="text-xs text-gray-500">Licenses</div>
    </div>
</div>

<div class="grid grid-cols-1 lg:grid-cols-2 gap-6 mb-8">
    <!-- Pricing -->
    <div class="bg-gray-900 border border-gray-800 rounded-2xl overflow-hidden">
        <div class="px-6 py-4 border-b border-gray-800"><h2 class="font-semibold text-white">Pricing</h2></div>
        <?php if (empty($pricings)): ?>
            <div class="px-6 py-6 text-center text-gray-500 text-sm">No pricing set.</div>
        <?php else: ?>
        <div class="divide-y divide-gray-800">
            <?php foreach ($pricings as $pr): ?>
            <div class="px-6 py-3 flex justify-between">
                <span class="text-gray-300 capitalize"><?= e(str_replace('_', ' ', $pr['license_type'])) ?><?= $pr['custom_days'] ? ' (' . $pr['custom_days'] . ' days)' : '' ?></span>
                <span class="text-white font-medium"><?= format_price($pr['price']) ?></span>
            </div>
            <?php endforeach; ?>
        </div>
        <?php endif; ?>
    </div>

    <!-- License Format -->
    <div class="bg-gray-900 border border-gray-800 rounded-2xl overflow-hidden">
        <div class="px-6 py-4 border-b border-gray-800"><h2 class="font-semibold text-white">License Key Format</h2></div>
        <div class="px-6 py-4">
            <?php if ($licenseFormat): ?>
            <dl class="space-y-2 text-sm">
                <div class="flex justify-between"><dt class="text-gray-400">Prefix</dt><dd class="text-white font-mono"><?= e($licenseFormat['prefix'] ?: '(none)') ?></dd></div>
                <div class="flex justify-between"><dt class="text-gray-400">Pattern</dt><dd class="text-white font-mono"><?= e($licenseFormat['format_pattern']) ?></dd></div>
                <div class="flex justify-between"><dt class="text-gray-400">Example</dt><dd class="text-brand-400 font-mono"><?= e(($licenseFormat['prefix'] ? $licenseFormat['prefix'] . '-' : '') . $licenseFormat['format_pattern']) ?></dd></div>
            </dl>
            <?php else: ?>
            <p class="text-gray-500 text-sm">Using default format: XXXX-XXXX-XXXX-XXXX</p>
            <?php endif; ?>
        </div>
    </div>
</div>

<!-- Versions -->
<div class="bg-gray-900 border border-gray-800 rounded-2xl overflow-hidden mb-8">
    <div class="px-6 py-4 border-b border-gray-800 flex items-center justify-between">
        <h2 class="font-semibold text-white">Versions</h2>
        <a href="/admin/plugins/<?= $id ?>/add-version" class="px-3 py-1.5 bg-brand-600 hover:bg-brand-500 text-white text-xs font-medium rounded-lg transition">Add Version</a>
    </div>
    <?php if (empty($versions)): ?>
        <div class="px-6 py-6 text-center text-gray-500 text-sm">No versions uploaded yet.</div>
    <?php else: ?>
    <div class="overflow-x-auto">
        <table class="w-full text-sm">
            <thead class="bg-gray-800/50 text-xs text-gray-400 uppercase">
                <tr><th class="px-6 py-3 text-left">Version</th><th class="px-6 py-3 text-left">File</th><th class="px-6 py-3 text-left">Size</th><th class="px-6 py-3 text-left">Released</th><th class="px-6 py-3 text-left">Status</th></tr>
            </thead>
            <tbody class="divide-y divide-gray-800">
                <?php foreach ($versions as $v): ?>
                <tr class="hover:bg-gray-800/30">
                    <td class="px-6 py-3"><span class="text-white font-medium">v<?= e($v['version']) ?></span><?php if ($v['is_latest']): ?> <span class="text-emerald-400 text-xs">(Latest)</span><?php endif; ?></td>
                    <td class="px-6 py-3 text-gray-400 text-xs font-mono"><?= e($v['file_name'] ?: '—') ?></td>
                    <td class="px-6 py-3 text-gray-400"><?= $v['file_size'] ? format_bytes($v['file_size']) : '—' ?></td>
                    <td class="px-6 py-3 text-gray-400"><?= $v['released_at'] ? format_date($v['released_at']) : '—' ?></td>
                    <td class="px-6 py-3"><?= $v['is_active'] ? '<span class="text-emerald-400 text-xs">Active</span>' : '<span class="text-gray-500 text-xs">Inactive</span>' ?></td>
                </tr>
                <?php if ($v['changelog']): ?>
                <tr class="bg-gray-800/10"><td colspan="5" class="px-6 py-2 text-xs text-gray-500"><?= nl2br(e($v['changelog'])) ?></td></tr>
                <?php endif; ?>
                <?php endforeach; ?>
            </tbody>
        </table>
    </div>
    <?php endif; ?>
</div>

<!-- Images -->
<div class="bg-gray-900 border border-gray-800 rounded-2xl overflow-hidden mb-8">
    <div class="px-6 py-4 border-b border-gray-800 flex items-center justify-between">
        <h2 class="font-semibold text-white">Images</h2>
        <a href="/admin/plugins/<?= $id ?>/add-image" class="px-3 py-1.5 bg-brand-600 hover:bg-brand-500 text-white text-xs font-medium rounded-lg transition">Add Image</a>
    </div>
    <?php if (empty($images)): ?>
        <div class="px-6 py-6 text-center text-gray-500 text-sm">No images uploaded.</div>
    <?php else: ?>
    <div class="p-6 grid grid-cols-2 sm:grid-cols-3 md:grid-cols-4 gap-4">
        <?php foreach ($images as $img): ?>
        <div class="relative group">
            <img src="/uploads/images/<?= e($img['image_path']) ?>" alt="<?= e($img['image_name']) ?>" class="w-full aspect-video object-cover rounded-lg border border-gray-700">
            <?php if ($img['is_featured']): ?><span class="absolute top-2 left-2 px-1.5 py-0.5 bg-amber-500 text-white text-xs rounded">Featured</span><?php endif; ?>
            <a href="/admin/plugins/delete-image/<?= $img['id'] ?>?plugin_id=<?= $id ?>" onclick="return confirm('Delete this image?')" class="absolute top-2 right-2 w-6 h-6 bg-red-600 text-white rounded-full flex items-center justify-center opacity-0 group-hover:opacity-100 transition text-xs">&times;</a>
        </div>
        <?php endforeach; ?>
    </div>
    <?php endif; ?>
</div>

<!-- Recent Licenses -->
<?php if (!empty($licenses)): ?>
<div class="bg-gray-900 border border-gray-800 rounded-2xl overflow-hidden mb-8">
    <div class="px-6 py-4 border-b border-gray-800"><h2 class="font-semibold text-white">Recent Licenses</h2></div>
    <div class="overflow-x-auto">
        <table class="w-full text-sm">
            <thead class="bg-gray-800/50 text-xs text-gray-400 uppercase">
                <tr><th class="px-6 py-3 text-left">User</th><th class="px-6 py-3 text-left">Key</th><th class="px-6 py-3 text-left">Type</th><th class="px-6 py-3 text-left">Status</th><th class="px-6 py-3 text-left">Expires</th></tr>
            </thead>
            <tbody class="divide-y divide-gray-800">
                <?php foreach ($licenses as $l): ?>
                <tr class="hover:bg-gray-800/30">
                    <td class="px-6 py-3"><a href="/admin/users/<?= $l['user_id'] ?>" class="text-brand-400 hover:text-brand-300"><?= e($l['user_name']) ?></a></td>
                    <td class="px-6 py-3 font-mono text-xs text-gray-300"><?= e($l['license_key']) ?></td>
                    <td class="px-6 py-3 text-gray-400 capitalize"><?= e(str_replace('_', ' ', $l['license_type'])) ?></td>
                    <td class="px-6 py-3"><span class="px-2 py-0.5 rounded-full text-xs <?= $l['status'] === 'active' ? 'bg-emerald-500/10 text-emerald-400' : ($l['status'] === 'expired' ? 'bg-amber-500/10 text-amber-400' : 'bg-red-500/10 text-red-400') ?>"><?= ucfirst($l['status']) ?></span></td>
                    <td class="px-6 py-3 text-gray-400"><?= $l['expires_at'] ? format_date($l['expires_at']) : 'Lifetime' ?></td>
                </tr>
                <?php endforeach; ?>
            </tbody>
        </table>
    </div>
</div>
<?php endif; ?>

<?php require BASE_PATH . '/includes/admin-footer.php'; ?>
