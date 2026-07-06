<?php
$pageTitle = 'View License - ' . SITE_NAME;
$id = (int)($_GET['id'] ?? 0);
$license = Database::fetch(
    "SELECT l.*, u.name as user_name, u.email as user_email, p.name as plugin_name, p.slug as plugin_slug
     FROM licenses l
     JOIN users u ON l.user_id = u.id
     JOIN plugins p ON l.plugin_id = p.id
     WHERE l.id = ?",
    [$id]
);
if (!$license) { set_flash('error', 'License not found.'); redirect('/admin/licenses'); }

$devices = Database::fetchAll(
    "SELECT * FROM device_activations WHERE license_id = ? ORDER BY activated_at DESC",
    [$id]
);

$order = $license['order_id'] ? Database::fetch("SELECT * FROM orders WHERE id = ?", [$license['order_id']]) : null;

require BASE_PATH . '/includes/admin-header.php';
?>

<div class="mb-6">
    <a href="/admin/licenses" class="text-sm text-gray-400 hover:text-white transition flex items-center gap-1 mb-2">
        <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M10 19l-7-7m0 0l7-7m-7 7h18"/></svg> Back to Licenses
    </a>
    <h1 class="text-2xl font-bold text-white">License Details</h1>
</div>

<div class="grid grid-cols-1 lg:grid-cols-3 gap-6 mb-8">
    <!-- License Info -->
    <div class="lg:col-span-2 bg-gray-900 border border-gray-800 rounded-2xl p-6">
        <div class="flex items-center justify-between mb-4">
            <h2 class="font-semibold text-white">License Information</h2>
            <?php
            $active = LicenseKeyGenerator::isActive($license);
            if ($active): ?>
                <span class="px-2.5 py-1 rounded-full text-xs font-medium bg-emerald-500/10 text-emerald-400 border border-emerald-500/20">Active</span>
            <?php elseif ($license['status'] === 'revoked'): ?>
                <span class="px-2.5 py-1 rounded-full text-xs font-medium bg-red-500/10 text-red-400 border border-red-500/20">Revoked</span>
            <?php else: ?>
                <span class="px-2.5 py-1 rounded-full text-xs font-medium bg-amber-500/10 text-amber-400 border border-amber-500/20">Expired</span>
            <?php endif; ?>
        </div>

        <div class="bg-gray-800 rounded-xl p-4 mb-4 text-center">
            <code class="text-lg font-mono font-bold text-brand-400 select-all tracking-wider"><?= e($license['license_key']) ?></code>
        </div>

        <dl class="grid grid-cols-2 gap-3 text-sm">
            <div><dt class="text-gray-500 text-xs uppercase">Type</dt><dd class="text-white capitalize mt-0.5"><?= e(str_replace('_', ' ', $license['license_type'])) ?></dd></div>
            <div><dt class="text-gray-500 text-xs uppercase">Status</dt><dd class="text-white capitalize mt-0.5"><?= e($license['status']) ?></dd></div>
            <div><dt class="text-gray-500 text-xs uppercase">Starts</dt><dd class="text-white mt-0.5"><?= format_datetime($license['starts_at']) ?></dd></div>
            <div><dt class="text-gray-500 text-xs uppercase">Expires</dt><dd class="text-white mt-0.5"><?= $license['expires_at'] ? format_datetime($license['expires_at']) : 'Never (Lifetime)' ?></dd></div>
            <div><dt class="text-gray-500 text-xs uppercase">Max Activations</dt><dd class="text-white mt-0.5"><?= $license['max_activations'] ?></dd></div>
            <div><dt class="text-gray-500 text-xs uppercase">Current Activations</dt><dd class="text-white mt-0.5"><?= $license['current_activations'] ?></dd></div>
            <div><dt class="text-gray-500 text-xs uppercase">Created</dt><dd class="text-white mt-0.5"><?= format_datetime($license['created_at']) ?></dd></div>
            <div><dt class="text-gray-500 text-xs uppercase">Last Activated</dt><dd class="text-white mt-0.5"><?= $license['last_activated_at'] ? format_datetime($license['last_activated_at']) : '—' ?></dd></div>
        </dl>
        <?php if ($license['notes']): ?>
        <div class="mt-4 pt-4 border-t border-gray-800">
            <dt class="text-gray-500 text-xs uppercase mb-1">Notes</dt>
            <dd class="text-gray-300 text-sm"><?= nl2br(e($license['notes'])) ?></dd>
        </div>
        <?php endif; ?>
    </div>

    <!-- Actions & Related -->
    <div class="space-y-6">
        <!-- Quick Actions -->
        <div class="bg-gray-900 border border-gray-800 rounded-2xl p-6">
            <h2 class="font-semibold text-white mb-4">Actions</h2>
            <div class="space-y-2">
                <?php if ($license['status'] === 'active'): ?>
                <form method="POST" action="/admin/licenses/action/<?= $id ?>">
                    <?= csrf_field() ?>
                    <input type="hidden" name="action" value="revoke">
                    <button type="submit" onclick="return confirm('Revoke this license?')" class="w-full px-4 py-2.5 bg-red-500/10 hover:bg-red-500/20 text-red-400 text-sm font-medium rounded-xl transition border border-red-500/20 text-left">Revoke License</button>
                </form>
                <?php endif; ?>
                <?php if ($license['status'] === 'revoked' || $license['status'] === 'expired'): ?>
                <form method="POST" action="/admin/licenses/action/<?= $id ?>">
                    <?= csrf_field() ?>
                    <input type="hidden" name="action" value="activate">
                    <button type="submit" class="w-full px-4 py-2.5 bg-emerald-500/10 hover:bg-emerald-500/20 text-emerald-400 text-sm font-medium rounded-xl transition border border-emerald-500/20 text-left">Reactivate License</button>
                </form>
                <?php endif; ?>
                <form method="POST" action="/admin/licenses/action/<?= $id ?>">
                    <?= csrf_field() ?>
                    <input type="hidden" name="action" value="extend">
                    <div class="flex gap-2">
                        <input type="number" name="days" value="30" min="1" class="flex-1 px-3 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white text-sm focus:outline-none focus:border-brand-500 transition">
                        <button type="submit" class="px-4 py-2.5 bg-brand-500/10 hover:bg-brand-500/20 text-brand-400 text-sm font-medium rounded-xl transition border border-brand-500/20">Extend Days</button>
                    </div>
                </form>
                <form method="POST" action="/admin/licenses/action/<?= $id ?>">
                    <?= csrf_field() ?>
                    <input type="hidden" name="action" value="reset_activations">
                    <button type="submit" onclick="return confirm('Reset all device activations?')" class="w-full px-4 py-2.5 bg-amber-500/10 hover:bg-amber-500/20 text-amber-400 text-sm font-medium rounded-xl transition border border-amber-500/20 text-left">Reset Activations</button>
                </form>
                <form method="POST" action="/admin/licenses/action/<?= $id ?>">
                    <?= csrf_field() ?>
                    <input type="hidden" name="action" value="set_max_activations">
                    <div class="flex gap-2">
                        <input type="number" name="max_activations" value="<?= $license['max_activations'] ?>" min="1" class="flex-1 px-3 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white text-sm focus:outline-none focus:border-brand-500 transition">
                        <button type="submit" class="px-4 py-2.5 bg-gray-700 hover:bg-gray-600 text-gray-300 text-sm font-medium rounded-xl transition">Set Max</button>
                    </div>
                </form>
            </div>
        </div>

        <!-- Related -->
        <div class="bg-gray-900 border border-gray-800 rounded-2xl p-6">
            <h2 class="font-semibold text-white mb-3">Related</h2>
            <div class="space-y-2 text-sm">
                <div class="flex justify-between"><span class="text-gray-400">User</span><a href="/admin/users/<?= $license['user_id'] ?>" class="text-brand-400 hover:text-brand-300"><?= e($license['user_name']) ?></a></div>
                <div class="flex justify-between"><span class="text-gray-400">Plugin</span><a href="/admin/plugins/<?= $license['plugin_id'] ?>" class="text-brand-400 hover:text-brand-300"><?= e($license['plugin_name']) ?></a></div>
                <?php if ($order): ?>
                <div class="flex justify-between"><span class="text-gray-400">Order</span><a href="/admin/orders/<?= $order['id'] ?>" class="text-brand-400 hover:text-brand-300"><?= e($order['order_number']) ?></a></div>
                <?php endif; ?>
            </div>
        </div>
    </div>
</div>

<!-- Device Activations -->
<div class="bg-gray-900 border border-gray-800 rounded-2xl overflow-hidden">
    <div class="px-6 py-4 border-b border-gray-800">
        <h2 class="font-semibold text-white">Device Activations (<?= count($devices) ?>)</h2>
    </div>
    <?php if (empty($devices)): ?>
        <div class="px-6 py-8 text-center text-gray-500 text-sm">No device activations.</div>
    <?php else: ?>
    <div class="overflow-x-auto">
        <table class="w-full text-sm">
            <thead class="bg-gray-800/50 text-xs text-gray-400 uppercase">
                <tr>
                    <th class="px-6 py-3 text-left">Device ID</th>
                    <th class="px-6 py-3 text-left">Name</th>
                    <th class="px-6 py-3 text-left">IP</th>
                    <th class="px-6 py-3 text-left">Activated</th>
                    <th class="px-6 py-3 text-left">Last Seen</th>
                    <th class="px-6 py-3 text-left">Status</th>
                </tr>
            </thead>
            <tbody class="divide-y divide-gray-800">
                <?php foreach ($devices as $d): ?>
                <tr class="hover:bg-gray-800/30">
                    <td class="px-6 py-3 font-mono text-xs text-gray-300"><?= e(substr($d['device_id'], 0, 30)) ?><?= strlen($d['device_id']) > 30 ? '...' : '' ?></td>
                    <td class="px-6 py-3 text-white"><?= e($d['device_name'] ?: '—') ?></td>
                    <td class="px-6 py-3 text-gray-400 font-mono text-xs"><?= e($d['ip_address'] ?? '—') ?></td>
                    <td class="px-6 py-3 text-gray-400"><?= format_datetime($d['activated_at']) ?></td>
                    <td class="px-6 py-3 text-gray-400"><?= $d['last_seen_at'] ? time_ago($d['last_seen_at']) : '—' ?></td>
                    <td class="px-6 py-3">
                        <?php if ($d['is_active']): ?>
                            <span class="px-2 py-0.5 rounded-full text-xs bg-emerald-500/10 text-emerald-400">Active</span>
                        <?php else: ?>
                            <span class="px-2 py-0.5 rounded-full text-xs bg-gray-500/10 text-gray-400">Deactivated</span>
                        <?php endif; ?>
                    </td>
                </tr>
                <?php endforeach; ?>
            </tbody>
        </table>
    </div>
    <?php endif; ?>
</div>

<?php require BASE_PATH . '/includes/admin-footer.php'; ?>
