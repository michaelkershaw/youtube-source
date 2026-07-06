<?php
require_admin();
$pageTitle = 'Clean Up Duplicate Licenses - ' . SITE_NAME;

// Find duplicate licenses
$duplicates = Database::fetchAll(
    "SELECT 
        l.user_id,
        u.email as user_email,
        u.name as user_name,
        l.plugin_id,
        p.name as plugin_name,
        COUNT(*) as duplicate_count,
        GROUP_CONCAT(l.id ORDER BY l.created_at DESC) as license_ids,
        GROUP_CONCAT(l.license_key ORDER BY l.created_at DESC) as license_keys,
        GROUP_CONCAT(l.status ORDER BY l.created_at DESC) as statuses,
        MAX(l.created_at) as latest_created,
        MIN(l.created_at) as oldest_created
     FROM licenses l
     JOIN users u ON l.user_id = u.id
     JOIN plugins p ON l.plugin_id = p.id
     GROUP BY l.user_id, l.plugin_id
     HAVING COUNT(*) > 1
     ORDER BY duplicate_count DESC, user_email, plugin_name"
);

// Get all duplicate license details
$allDuplicates = Database::fetchAll(
    "SELECT 
        l.id,
        l.license_key,
        l.status,
        l.license_type,
        l.created_at,
        l.expires_at,
        l.max_activations,
        l.devices,
        l.max_activations_updated,
        u.email as user_email,
        u.name as user_name,
        p.name as plugin_name
     FROM licenses l
     JOIN users u ON l.user_id = u.id
     JOIN plugins p ON l.plugin_id = p.id
     WHERE (l.user_id, l.plugin_id) IN (
         SELECT user_id, plugin_id
         FROM licenses
         GROUP BY user_id, plugin_id
         HAVING COUNT(*) > 1
     )
     ORDER BY l.user_id, l.plugin_id, l.created_at DESC"
);

require BASE_PATH . '/includes/admin-header.php';
?>

<div class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-8">
    <div class="mb-8">
        <h1 class="text-2xl font-bold text-white mb-2">Duplicate Licenses</h1>
        <p class="text-gray-400">Found <?= count($duplicates) ?> users with duplicate licenses</p>
    </div>

    <?php if (!empty($duplicates)): ?>
    <div class="bg-yellow-500/10 border border-yellow-500/20 rounded-xl p-4 mb-6">
        <div class="flex items-center gap-3">
            <svg class="w-5 h-5 text-yellow-400" fill="currentColor" viewBox="0 0 20 20"><path fill-rule="evenodd" d="M8.257 3.099c.765-1.36 2.722-1.36 3.486 0l5.58 9.92c.75 1.334-.213 2.98-1.742 2.98H4.42c-1.53 0-2.493-1.646-1.743-2.98l5.58-9.92zM11 13a1 1 0 11-2 0 1 1 0 012 0zm-1-8a1 1 0 00-1 1v3a1 1 0 002 0V6a1 1 0 00-1-1z" clip-rule="evenodd"/></svg>
            <div>
                <h3 class="text-yellow-400 font-semibold">Before cleaning up duplicates</h3>
                <p class="text-yellow-200 text-sm">Make sure to backup your database first. This will delete extra duplicate licenses, keeping only the most recent active one for each user/plugin pair.</p>
            </div>
        </div>
    </div>

    <div class="space-y-6">
        <?php foreach ($duplicates as $dup): ?>
        <div class="bg-gray-900 border border-gray-800 rounded-xl p-6">
            <div class="flex items-center justify-between mb-4">
                <div>
                    <h3 class="text-lg font-semibold text-white"><?= e($dup['user_name']) ?> (<?= e($dup['user_email']) ?>)</h3>
                    <p class="text-gray-400"><?= e($dup['plugin_name']) ?> - <?= $dup['duplicate_count'] ?> duplicates</p>
                </div>
                <span class="px-3 py-1 bg-red-500/10 text-red-400 text-sm font-medium rounded-full">
                    <?= $dup['duplicate_count'] - 1 ?> extra licenses
                </span>
            </div>

            <div class="space-y-2">
                <?php 
                $ids = explode(',', $dup['license_ids']);
                $keys = explode(',', $dup['license_keys']);
                $statuses = explode(',', $dup['statuses']);
                
                foreach ($ids as $i => $id): 
                    $license = array_filter($allDuplicates, fn($l) => $l['id'] == trim($id))[0] ?? null;
                    if ($license):
                ?>
                <div class="flex items-center justify-between p-3 bg-gray-800 rounded-lg">
                    <div class="flex items-center gap-3">
                        <code class="bg-gray-700 px-2 py-1 rounded text-xs"><?= e(trim($keys[$i])) ?></code>
                        <span class="px-2 py-0.5 text-xs rounded <?= $license['status'] === 'active' ? 'bg-emerald-500/10 text-emerald-400' : 'bg-gray-600/10 text-gray-400' ?>">
                            <?= e($license['status']) ?>
                        </span>
                        <span class="text-xs text-gray-400"><?= e($license['license_type']) ?></span>
                        <span class="text-xs text-gray-500"><?= format_date($license['created_at']) ?></span>
                    </div>
                    <div class="text-xs text-gray-400">
                        <?= $license['devices'] ?> / <?= $license['max_activations_updated'] ?? $license['max_activations'] ?> devices
                    </div>
                </div>
                <?php endif; endforeach; ?>
            </div>
        </div>
        <?php endforeach; ?>
    </div>

    <div class="mt-8 flex gap-4">
        <form method="POST" action="/admin/cleanup-duplicates-execute" onsubmit="return confirm('This will delete <?= array_sum(array_map(fn($d) => $d['duplicate_count'] - 1, $duplicates)) ?> duplicate licenses. Are you sure?')">
            <button type="submit" class="px-6 py-3 bg-red-600 hover:bg-red-500 text-white font-semibold rounded-xl transition">
                Clean Up All Duplicates
            </button>
        </form>
        <a href="/admin/licenses" class="px-6 py-3 bg-gray-700 hover:bg-gray-600 text-white font-semibold rounded-xl transition">
            Cancel
        </a>
    </div>
    <?php else: ?>
    <div class="bg-emerald-500/10 border border-emerald-500/20 rounded-xl p-6 text-center">
        <svg class="w-12 h-12 text-emerald-400 mx-auto mb-3" fill="currentColor" viewBox="0 0 20 20"><path fill-rule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zm3.707-9.293a1 1 0 00-1.414-1.414L9 10.586 7.707 9.293a1 1 0 00-1.414 1.414l2 2a1 1 0 001.414 0l4-4z" clip-rule="evenodd"/></svg>
        <h3 class="text-emerald-400 font-semibold text-lg mb-2">No Duplicate Licenses Found</h3>
        <p class="text-emerald-200">All licenses are unique.</p>
    </div>
    <?php endif; ?>
</div>

<?php require BASE_PATH . '/includes/admin-footer.php'; ?>
