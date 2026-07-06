<?php
require_login();
$pageTitle = 'My Dashboard - ' . SITE_NAME;
$user = current_user();

// Get user's licenses with plugin info (LEFT JOIN to preserve licenses even if plugin deleted)
$licenses = Database::fetchAll(
    "SELECT DISTINCT l.*, p.name as plugin_name, p.slug as plugin_slug,
        (SELECT version FROM plugin_versions WHERE plugin_id = l.plugin_id AND is_latest = 1 LIMIT 1) as latest_version,
        (SELECT id FROM plugin_versions WHERE plugin_id = l.plugin_id AND is_latest = 1 LIMIT 1) as latest_version_id
     FROM licenses l
     LEFT JOIN plugins p ON l.plugin_id = p.id
     WHERE l.user_id = ?
     ORDER BY l.created_at DESC",
    [$user['id']]
);

// Remove any duplicate licenses (same user + plugin, keep most recent)
$uniqueLicenses = [];
$seenPlugins = [];
foreach ($licenses as $lic) {
    $key = $lic['user_id'] . '_' . $lic['plugin_id'];
    if (!isset($seenPlugins[$key])) {
        $lic['current_max_activations'] = $lic['max_activations_updated'] ?? $lic['max_activations'];
        $uniqueLicenses[] = $lic;
        $seenPlugins[$key] = true;
    }
}
$licenses = $uniqueLicenses;

// Get order history
$orders = Database::fetchAll(
    "SELECT o.*, p.name as plugin_name, p.slug as plugin_slug
     FROM orders o
     JOIN plugins p ON o.plugin_id = p.id
     WHERE o.user_id = ?
     ORDER BY o.created_at DESC LIMIT 20",
    [$user['id']]
);

// Get device activations for user's licenses
$licenseIds = array_column($licenses, 'id');
$devices = [];
if (!empty($licenseIds)) {
    $placeholders = implode(',', array_fill(0, count($licenseIds), '?'));
    $devices = Database::fetchAll(
        "SELECT da.*, l.license_key, p.name as plugin_name
         FROM device_activations da
         JOIN licenses l ON da.license_id = l.id
         JOIN plugins p ON l.plugin_id = p.id
         WHERE da.license_id IN ({$placeholders}) AND da.is_active = 1
         ORDER BY da.activated_at DESC",
        $licenseIds
    );
}

require BASE_PATH . '/includes/header.php';
?>

<div class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-10">
    <!-- Header -->
    <div class="flex flex-col sm:flex-row sm:items-center sm:justify-between gap-4 mb-10">
        <div>
            <h1 class="text-3xl font-bold text-white">My Dashboard</h1>
            <p class="text-gray-400 mt-1">Welcome back, <?= e($user['name']) ?></p>
        </div>
        <a href="/plugins" class="inline-flex items-center gap-2 px-5 py-2.5 bg-brand-600 hover:bg-brand-500 text-white text-sm font-medium rounded-xl transition shadow-lg shadow-brand-600/20">
            <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 6v6m0 0v6m0-6h6m-6 0H6"/></svg>
            Browse Plugins
        </a>
    </div>

    <!-- Quick Stats -->
    <div class="grid grid-cols-1 sm:grid-cols-4 gap-4 mb-10">
        <div class="bg-gray-900 border border-gray-800 rounded-xl p-5">
            <div class="flex items-center gap-3">
                <div class="w-10 h-10 bg-brand-500/10 rounded-lg flex items-center justify-center">
                    <svg class="w-5 h-5 text-brand-400" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M15 7a2 2 0 012 2m4 0a6 6 0 01-7.743 5.743L11 17H9v2H7v2H4a1 1 0 01-1-1v-2.586a1 1 0 01.293-.707l5.964-5.964A6 6 0 1121 9z"/></svg>
                </div>
                <div>
                    <div class="text-2xl font-bold text-white"><?= count($licenses) ?></div>
                    <div class="text-xs text-gray-500">Licenses</div>
                </div>
            </div>
        </div>
        <div class="bg-gray-900 border border-gray-800 rounded-xl p-5">
            <div class="flex items-center gap-3">
                <div class="w-10 h-10 bg-emerald-500/10 rounded-lg flex items-center justify-center">
                    <svg class="w-5 h-5 text-emerald-400" fill="currentColor" viewBox="0 0 20 20"><path fill-rule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zm3.707-9.293a1 1 0 00-1.414-1.414L9 10.586 7.707 9.293a1 1 0 00-1.414 1.414l2 2a1 1 0 001.414 0l4-4z" clip-rule="evenodd"/></svg>
                </div>
                <div>
                    <div class="text-2xl font-bold text-white"><?= count(array_filter($licenses, fn($l) => LicenseKeyGenerator::isActive($l))) ?></div>
                    <div class="text-xs text-gray-500">Active</div>
                </div>
            </div>
        </div>
        <div class="bg-gray-900 border border-gray-800 rounded-xl p-5">
            <div class="flex items-center gap-3">
                <div class="w-10 h-10 bg-purple-500/10 rounded-lg flex items-center justify-center">
                    <svg class="w-5 h-5 text-purple-400" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M9 5H7a2 2 0 00-2 2v12a2 2 0 002 2h10a2 2 0 002-2V7a2 2 0 00-2-2h-2M9 5a2 2 0 002 2h2a2 2 0 002-2M9 5a2 2 0 012-2h2a2 2 0 012 2"/></svg>
                </div>
                <div>
                    <div class="text-2xl font-bold text-white"><?= count($orders) ?></div>
                    <div class="text-xs text-gray-500">Orders</div>
                </div>
            </div>
        </div>
        <div class="bg-gray-900 border border-gray-800 rounded-xl p-5">
            <div class="flex items-center gap-3">
                <div class="w-10 h-10 bg-amber-500/10 rounded-lg flex items-center justify-center">
                    <svg class="w-5 h-5 text-amber-400" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M9.75 17L9 20l-1 1h8l-1-1-.75-3M3 13h18M5 17h14a2 2 0 002-2V5a2 2 0 00-2-2H5a2 2 0 00-2 2v10a2 2 0 002 2z"/></svg>
                </div>
                <div>
                    <div class="text-2xl font-bold text-white"><?= count($devices) ?></div>
                    <div class="text-xs text-gray-500">Active Devices</div>
                </div>
            </div>
        </div>
    </div>

    <!-- My Licenses -->
    <div class="bg-gray-900 border border-gray-800 rounded-2xl overflow-hidden mb-10">
        <div class="px-6 py-4 border-b border-gray-800 flex items-center justify-between">
            <h2 class="text-lg font-semibold text-white">My Licenses</h2>
        </div>
        <?php if (empty($licenses)): ?>
        <div class="px-6 py-12 text-center">
            <svg class="w-12 h-12 text-gray-700 mx-auto mb-3" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="1" d="M15 7a2 2 0 012 2m4 0a6 6 0 01-7.743 5.743L11 17H9v2H7v2H4a1 1 0 01-1-1v-2.586a1 1 0 01.293-.707l5.964-5.964A6 6 0 1121 9z"/></svg>
            <p class="text-gray-400">You don't have any licenses yet.</p>
            <a href="/plugins" class="text-brand-400 hover:text-brand-300 text-sm font-medium mt-1 inline-block">Browse plugins</a>
        </div>
        <?php else: ?>
        <div class="overflow-x-auto">
            <table class="w-full text-sm text-left">
                <thead class="bg-gray-800/50 text-xs text-gray-400 uppercase tracking-wider">
                    <tr>
                        <th class="px-6 py-3">Plugin</th>
                        <th class="px-6 py-3">License Key</th>
                        <th class="px-6 py-3">Type</th>
                        <th class="px-6 py-3">Expires</th>
                        <th class="px-6 py-3">Devices</th>
                        <th class="px-6 py-3">Status</th>
                        <th class="px-6 py-3">Version</th>
                        <th class="px-6 py-3">Action</th>
                    </tr>
                </thead>
                <tbody class="divide-y divide-gray-800">
                    <?php foreach ($licenses as $lic): ?>
                    <?php $active = LicenseKeyGenerator::isActive($lic); ?>
                    <tr class="hover:bg-gray-800/30 transition">
                        <td class="px-6 py-4">
                            <?php if ($lic['plugin_slug']): ?>
                                <a href="/plugin/<?= e($lic['plugin_slug']) ?>?from=dashboard" class="font-medium text-brand-400 hover:text-brand-300 transition"><?= e($lic['plugin_name']) ?></a>
                            <?php else: ?>
                                <span class="font-medium text-gray-500">[Deleted Plugin]</span>
                            <?php endif; ?>
                        </td>
                        <td class="px-6 py-4">
                            <code class="font-mono text-xs bg-gray-800 text-gray-300 px-2 py-1 rounded select-all cursor-pointer"><?= e($lic['license_key']) ?></code>
                        </td>
                        <td class="px-6 py-4 text-gray-400 capitalize"><?= e(str_replace('_', ' ', $lic['license_type'])) ?></td>
                        <td class="px-6 py-4">
                            <?php if ($lic['expires_at']): ?>
                                <?php $daysLeft = days_until($lic['expires_at']); ?>
                                <span class="text-gray-300"><?= format_date($lic['expires_at']) ?></span>
                                <?php if ($daysLeft < 0): ?>
                                    <span class="text-red-400 text-xs block mt-0.5">Expired</span>
                                <?php elseif ($daysLeft <= 30): ?>
                                    <span class="text-amber-400 text-xs block mt-0.5"><?= $daysLeft ?> days left</span>
                                <?php endif; ?>
                            <?php else: ?>
                                <span class="text-emerald-400 text-sm">Lifetime</span>
                            <?php endif; ?>
                        </td>
                        <td class="px-6 py-4 text-gray-400"><?= $lic['devices'] ?> / <?= $lic['current_max_activations'] ?></td>
                        <td class="px-6 py-4">
                            <?php if ($active): ?>
                                <span class="inline-flex items-center px-2.5 py-0.5 rounded-full text-xs font-medium bg-emerald-500/10 text-emerald-400 border border-emerald-500/20">Active</span>
                            <?php elseif ($lic['status'] === 'expired' || ($lic['expires_at'] && strtotime($lic['expires_at']) < time())): ?>
                                <span class="inline-flex items-center px-2.5 py-0.5 rounded-full text-xs font-medium bg-amber-500/10 text-amber-400 border border-amber-500/20">Expired</span>
                            <?php else: ?>
                                <span class="inline-flex items-center px-2.5 py-0.5 rounded-full text-xs font-medium bg-red-500/10 text-red-400 border border-red-500/20">Revoked</span>
                            <?php endif; ?>
                        </td>
                        <td class="px-6 py-4 text-gray-400">
                            <?= $lic['latest_version'] ? 'v' . e($lic['latest_version']) : '—' ?>
                        </td>
                        <td class="px-6 py-4">
                            <div class="flex items-center gap-2">
                                <?php if ($lic['latest_version_id']): ?>
                                    <a href="/download/<?= $lic['plugin_id'] ?>/<?= $lic['latest_version_id'] ?>" class="text-brand-400 hover:text-brand-300 transition" title="Download">
                                        <svg class="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M4 16v1a3 3 0 003 3h10a3 3 0 003-3v-1m-4-4l-4 4m0 0l-4-4m4 4V4"/></svg>
                                    </a>
                                <?php endif; ?>
                                <?php if ($lic['license_type'] === 'lifetime' && $lic['plugin_slug']): ?>
                                    <a href="/upgrade-devices?license=<?= e($lic['license_key']) ?>&back=<?= urlencode('/dashboard') ?>" class="text-purple-400 hover:text-purple-300 transition" title="Upgrade Device Slots">
                                        <svg class="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 6v6m0 0v6m0-6h6m-6 0H6"/></svg>
                                    </a>
                                <?php endif; ?>
                            </div>
                        </td>
                    </tr>
                    <?php endforeach; ?>
                </tbody>
            </table>
        </div>
        <?php endif; ?>
    </div>

    <!-- Active Devices -->
    <?php if (!empty($devices)): ?>
    <div class="bg-gray-900 border border-gray-800 rounded-2xl overflow-hidden mb-10">
        <div class="px-6 py-4 border-b border-gray-800">
            <h2 class="text-lg font-semibold text-white">Active Devices</h2>
            <p class="text-xs text-gray-500 mt-1">Deactivate devices you no longer use to free up activation slots.</p>
        </div>
        <div class="overflow-x-auto">
            <table class="w-full text-sm text-left">
                <thead class="bg-gray-800/50 text-xs text-gray-400 uppercase tracking-wider">
                    <tr>
                        <th class="px-6 py-3">Device</th>
                        <th class="px-6 py-3">Plugin</th>
                        <th class="px-6 py-3">Activated</th>
                        <th class="px-6 py-3">Last Seen</th>
                        <th class="px-6 py-3">IP Address</th>
                        <th class="px-6 py-3">Action</th>
                    </tr>
                </thead>
                <tbody class="divide-y divide-gray-800">
                    <?php foreach ($devices as $dev): ?>
                    <tr class="hover:bg-gray-800/30 transition">
                        <td class="px-6 py-4">
                            <div class="font-medium text-white"><?= e($dev['device_name'] ?: $dev['device_id']) ?></div>
                            <div class="text-xs text-gray-500 font-mono"><?= e(substr($dev['device_id'], 0, 20)) ?>...</div>
                        </td>
                        <td class="px-6 py-4 text-gray-400"><?= e($dev['plugin_name']) ?></td>
                        <td class="px-6 py-4 text-gray-400"><?= format_date($dev['activated_at']) ?></td>
                        <td class="px-6 py-4 text-gray-400"><?= $dev['last_seen_at'] ? time_ago($dev['last_seen_at']) : '—' ?></td>
                        <td class="px-6 py-4 text-gray-500 font-mono text-xs"><?= e($dev['ip_address'] ?? '—') ?></td>
                        <td class="px-6 py-4">
                            <form method="POST" action="/dashboard/deactivate-device/<?= $dev['id'] ?>" onsubmit="return confirm('Deactivate this device?')">
                                <?= csrf_field() ?>
                                <button type="submit" class="px-3 py-1.5 bg-red-500/10 hover:bg-red-500/20 text-red-400 text-xs font-medium rounded-lg transition border border-red-500/20">
                                    Deactivate
                                </button>
                            </form>
                        </td>
                    </tr>
                    <?php endforeach; ?>
                </tbody>
            </table>
        </div>
    </div>
    <?php endif; ?>

    <!-- Order History -->
    <div class="bg-gray-900 border border-gray-800 rounded-2xl overflow-hidden">
        <div class="px-6 py-4 border-b border-gray-800">
            <h2 class="text-lg font-semibold text-white">Order History</h2>
        </div>
        <?php if (empty($orders)): ?>
        <div class="px-6 py-12 text-center">
            <p class="text-gray-400">No orders yet.</p>
        </div>
        <?php else: ?>
        <div class="overflow-x-auto">
            <table class="w-full text-sm text-left">
                <thead class="bg-gray-800/50 text-xs text-gray-400 uppercase tracking-wider">
                    <tr>
                        <th class="px-6 py-3">Order #</th>
                        <th class="px-6 py-3">Plugin</th>
                        <th class="px-6 py-3">License Type</th>
                        <th class="px-6 py-3">Amount</th>
                        <th class="px-6 py-3">Status</th>
                        <th class="px-6 py-3">Date</th>
                    </tr>
                </thead>
                <tbody class="divide-y divide-gray-800">
                    <?php foreach ($orders as $order): ?>
                    <tr class="hover:bg-gray-800/30 transition">
                        <td class="px-6 py-4 font-medium text-white font-mono text-xs"><?= e($order['order_number']) ?></td>
                        <td class="px-6 py-4"><a href="/plugin/<?= e($order['plugin_slug']) ?>" class="text-brand-400 hover:text-brand-300 transition"><?= e($order['plugin_name']) ?></a></td>
                        <td class="px-6 py-4 text-gray-400 capitalize"><?= e(str_replace('_', ' ', $order['license_type'])) ?></td>
                        <td class="px-6 py-4 text-white font-medium"><?= format_price($order['price']) ?></td>
                        <td class="px-6 py-4">
                            <?php
                            $statusColors = [
                                'completed' => 'bg-emerald-500/10 text-emerald-400 border-emerald-500/20',
                                'pending' => 'bg-amber-500/10 text-amber-400 border-amber-500/20',
                                'refunded' => 'bg-red-500/10 text-red-400 border-red-500/20',
                                'failed' => 'bg-gray-500/10 text-gray-400 border-gray-500/20',
                            ];
                            $sc = $statusColors[$order['status']] ?? $statusColors['pending'];
                            ?>
                            <span class="inline-flex items-center px-2.5 py-0.5 rounded-full text-xs font-medium <?= $sc ?> border"><?= ucfirst($order['status']) ?></span>
                        </td>
                        <td class="px-6 py-4 text-gray-400"><?= format_date($order['created_at']) ?></td>
                    </tr>
                    <?php endforeach; ?>
                </tbody>
            </table>
        </div>
        <?php endif; ?>
    </div>
</div>

<?php require BASE_PATH . '/includes/footer.php'; ?>
