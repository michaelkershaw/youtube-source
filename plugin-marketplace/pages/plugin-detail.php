<?php
$slug = $_GET['slug'] ?? '';
$id = $_GET['id'] ?? '';

// Find plugin by slug or ID
if ($slug) {
    $plugin = Database::fetch("SELECT * FROM plugins WHERE slug = ? AND is_active = 1", [$slug]);
} elseif ($id) {
    $plugin = Database::fetch("SELECT * FROM plugins WHERE id = ? AND is_active = 1", [$id]);
} else {
    $plugin = null;
}

if (!$plugin) {
    http_response_code(404);
    require BASE_PATH . '/pages/404.php';
    return;
}

$pageTitle = e($plugin['name']) . ' - ' . SITE_NAME;
$images = Database::fetchAll("SELECT * FROM plugin_images WHERE plugin_id = ? ORDER BY sort_order", [$plugin['id']]);
$versions = Database::fetchAll("SELECT * FROM plugin_versions WHERE plugin_id = ? AND is_active = 1 ORDER BY released_at DESC, created_at DESC", [$plugin['id']]);
$pricings = Database::fetchAll("SELECT * FROM plugin_pricings WHERE plugin_id = ? AND is_active = 1 ORDER BY price ASC", [$plugin['id']]);
$latestVersion = Database::fetch("SELECT * FROM plugin_versions WHERE plugin_id = ? AND is_latest = 1", [$plugin['id']]);

// Check if current user owns this plugin
$userLicense = null;
$fromDashboard = isset($_GET['from']) && $_GET['from'] === 'dashboard';
if (is_logged_in()) {
    $userLicense = Database::fetch(
        "SELECT * FROM licenses WHERE user_id = ? AND plugin_id = ? AND status = 'active' AND (expires_at IS NULL OR expires_at > NOW()) ORDER BY created_at DESC LIMIT 1",
        [current_user_id(), $plugin['id']]
    );
}

require BASE_PATH . '/includes/header.php';
?>

<div class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-12">
    <!-- Breadcrumb -->
    <nav class="flex items-center gap-2 text-sm text-gray-500 mb-8">
        <a href="/" class="hover:text-gray-300 transition">Home</a>
        <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M9 5l7 7-7 7"/></svg>
        <a href="/plugins" class="hover:text-gray-300 transition">Plugins</a>
        <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M9 5l7 7-7 7"/></svg>
        <span class="text-gray-400"><?= e($plugin['name']) ?></span>
    </nav>

    <div class="grid grid-cols-1 lg:grid-cols-3 gap-10">
        <!-- Left: Plugin Info -->
        <div class="lg:col-span-2 space-y-8">
            <!-- Image Gallery -->
            <?php if (!empty($images)): ?>
            <div x-data="{ activeImage: 0 }" class="space-y-3">
                <div class="aspect-video bg-gray-900 border border-gray-800 rounded-2xl overflow-hidden">
                    <?php foreach ($images as $i => $img): ?>
                    <img x-show="activeImage === <?= $i ?>" src="/uploads/images/<?= e($img['image_path']) ?>" alt="<?= e($img['caption'] ?? $plugin['name']) ?>" class="w-full h-full object-contain">
                    <?php endforeach; ?>
                </div>
                <?php if (count($images) > 1): ?>
                <div class="flex gap-2 overflow-x-auto pb-2">
                    <?php foreach ($images as $i => $img): ?>
                    <button @click="activeImage = <?= $i ?>" :class="activeImage === <?= $i ?> ? 'border-brand-500 ring-2 ring-brand-500/30' : 'border-gray-700 hover:border-gray-600'" class="flex-shrink-0 w-20 h-14 rounded-lg border overflow-hidden transition">
                        <img src="/uploads/images/<?= e($img['image_path']) ?>" alt="" class="w-full h-full object-cover">
                    </button>
                    <?php endforeach; ?>
                </div>
                <?php endif; ?>
            </div>
            <?php else: ?>
            <div class="aspect-video bg-gray-900 border border-gray-800 rounded-2xl flex items-center justify-center">
                <svg class="w-24 h-24 text-gray-700" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="1" d="M20 7l-8-4-8 4m16 0l-8 4m8-4v10l-8 4m0-10L4 7m8 4v10M4 7v10l8 4"/></svg>
            </div>
            <?php endif; ?>

            <!-- Plugin Name & Description -->
            <div>
                <div class="flex items-center gap-3 mb-4">
                    <h1 class="text-3xl font-bold text-white"><?= e($plugin['name']) ?></h1>
                    <?php if ($plugin['is_featured']): ?>
                        <span class="px-2.5 py-1 bg-amber-500/90 text-white text-xs font-bold rounded-lg">FEATURED</span>
                    <?php endif; ?>
                </div>
                <?php if ($plugin['category']): ?>
                    <span class="inline-block px-3 py-1 bg-gray-800 text-gray-400 text-xs rounded-lg mb-4"><?= e($plugin['category']) ?></span>
                <?php endif; ?>
                <div class="prose prose-invert max-w-none">
                    <p class="text-gray-300 leading-relaxed"><?= nl2br(e($plugin['description'])) ?></p>
                    <?php if ($plugin['full_description']): ?>
                        <div class="mt-6 text-gray-400 leading-relaxed"><?= nl2br(e($plugin['full_description'])) ?></div>
                    <?php endif; ?>
                </div>
            </div>

            <!-- Stats -->
            <div class="grid grid-cols-2 sm:grid-cols-4 gap-4">
                <div class="bg-gray-900 border border-gray-800 rounded-xl p-4 text-center">
                    <div class="text-xl font-bold text-white"><?= $latestVersion ? 'v' . e($latestVersion['version']) : '—' ?></div>
                    <div class="text-xs text-gray-500 mt-1">Latest Version</div>
                </div>
                <div class="bg-gray-900 border border-gray-800 rounded-xl p-4 text-center">
                    <div class="text-xl font-bold text-white"><?= number_format($plugin['downloads_count']) ?></div>
                    <div class="text-xs text-gray-500 mt-1">Downloads</div>
                </div>
                <div class="bg-gray-900 border border-gray-800 rounded-xl p-4 text-center">
                    <div class="text-xl font-bold text-white"><?= number_format($plugin['sales_count']) ?></div>
                    <div class="text-xs text-gray-500 mt-1">Sales</div>
                </div>
                <div class="bg-gray-900 border border-gray-800 rounded-xl p-4 text-center">
                    <div class="text-xl font-bold text-white"><?= count($versions) ?></div>
                    <div class="text-xs text-gray-500 mt-1">Versions</div>
                </div>
            </div>

            <!-- Versions & Changelog -->
            <?php if (!empty($versions)): ?>
            <div class="bg-gray-900 border border-gray-800 rounded-2xl overflow-hidden">
                <div class="px-6 py-4 border-b border-gray-800">
                    <h2 class="text-lg font-semibold text-white">Versions & Changelog</h2>
                </div>
                <div class="divide-y divide-gray-800">
                    <?php foreach ($versions as $ver): ?>
                    <div class="px-6 py-4">
                        <div class="flex items-center justify-between">
                            <div class="flex items-center gap-3">
                                <span class="text-white font-semibold">v<?= e($ver['version']) ?></span>
                                <?php if ($ver['is_latest']): ?>
                                    <span class="px-2 py-0.5 bg-emerald-500/10 text-emerald-400 text-xs font-medium rounded-md">Latest</span>
                                <?php endif; ?>
                            </div>
                            <div class="flex items-center gap-3">
                                <?php if ($ver['released_at']): ?>
                                    <span class="text-xs text-gray-500"><?= format_date($ver['released_at']) ?></span>
                                <?php endif; ?>
                                <?php if ($ver['file_size']): ?>
                                    <span class="text-xs text-gray-500"><?= format_bytes($ver['file_size']) ?></span>
                                <?php endif; ?>
                                <?php if ($userLicense && $ver['file_path']): ?>
                                    <a href="/download/<?= $plugin['id'] ?>/<?= $ver['id'] ?>" class="px-3 py-1.5 bg-brand-600 hover:bg-brand-500 text-white text-xs font-medium rounded-lg transition">Download</a>
                                <?php endif; ?>
                            </div>
                        </div>
                        <?php if ($ver['changelog']): ?>
                            <p class="text-sm text-gray-400 mt-2"><?= nl2br(e($ver['changelog'])) ?></p>
                        <?php endif; ?>
                    </div>
                    <?php endforeach; ?>
                </div>
            </div>
            <?php endif; ?>
        </div>

        <!-- Right Sidebar: Pricing & Actions -->
        <div class="space-y-6">
            <!-- Ownership Status -->
            <?php if ($userLicense && $fromDashboard): ?>
            <div class="bg-emerald-500/5 border border-emerald-500/20 rounded-2xl p-6">
                <div class="flex items-center gap-3 mb-3">
                    <div class="w-10 h-10 bg-emerald-500/10 rounded-xl flex items-center justify-center">
                        <svg class="w-5 h-5 text-emerald-400" fill="currentColor" viewBox="0 0 20 20"><path fill-rule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zm3.707-9.293a1 1 0 00-1.414-1.414L9 10.586 7.707 9.293a1 1 0 00-1.414 1.414l2 2a1 1 0 001.414 0l4-4z" clip-rule="evenodd"/></svg>
                    </div>
                    <div>
                        <h3 class="font-semibold text-emerald-400">You Own This Plugin</h3>
                        <p class="text-xs text-gray-400">License: <?= e($userLicense['license_type']) ?></p>
                    </div>
                </div>
                <div class="space-y-2 text-sm">
                    <div class="flex justify-between">
                        <span class="text-gray-400">License Key</span>
                        <code class="text-emerald-400 font-mono text-xs bg-gray-800 px-2 py-0.5 rounded select-all"><?= e($userLicense['license_key']) ?></code>
                    </div>
                    <?php if ($userLicense['expires_at']): ?>
                    <div class="flex justify-between">
                        <span class="text-gray-400">Expires</span>
                        <span class="text-white"><?= format_date($userLicense['expires_at']) ?></span>
                    </div>
                    <?php else: ?>
                    <div class="flex justify-between">
                        <span class="text-gray-400">Expires</span>
                        <span class="text-emerald-400">Lifetime</span>
                    </div>
                    <?php endif; ?>
                </div>
                <?php if ($latestVersion && $latestVersion['file_path']): ?>
                <a href="/download/<?= $plugin['id'] ?>/<?= $latestVersion['id'] ?>" class="mt-4 w-full flex items-center justify-center gap-2 px-4 py-3 bg-emerald-600 hover:bg-emerald-500 text-white font-semibold rounded-xl transition">
                    <svg class="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M4 16v1a3 3 0 003 3h10a3 3 0 003-3v-1m-4-4l-4 4m0 0l-4-4m4 4V4"/></svg>
                    Download Latest (v<?= e($latestVersion['version']) ?>)
                </a>
                <?php endif; ?>
                
                <?php if ($userLicense && $userLicense['license_type'] === 'lifetime'): ?>
                <div class="mt-3 pt-3 border-t border-emerald-500/20">
                    <div class="flex items-center justify-between mb-2">
                        <span class="text-xs text-gray-400">Device Slots</span>
                        <span class="text-xs text-emerald-400"><?= $userLicense['devices'] ?> / <?= $userLicense['max_activations_updated'] ?? $userLicense['max_activations'] ?></span>
                    </div>
                    <a href="/upgrade-devices?license=<?= e($userLicense['license_key']) ?>&back=<?= urlencode('/plugin/' . $plugin['slug']) ?>" class="w-full flex items-center justify-center gap-2 px-4 py-2.5 bg-purple-600 hover:bg-purple-500 text-white text-sm font-medium rounded-xl transition">
                        <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 6v6m0 0v6m0-6h6m-6 0H6"/></svg>
                        Upgrade Device Slots
                    </a>
                </div>
                <?php endif; ?>
            </div>
            <?php endif; ?>

            <!-- Pricing -->
            <?php if (!empty($pricings)): ?>
            <?php if ($userLicense && !$fromDashboard): ?>
            <div class="bg-blue-500/5 border border-blue-500/20 rounded-2xl p-6 mb-6">
                <div class="flex items-center gap-3 mb-3">
                    <div class="w-10 h-10 bg-blue-500/10 rounded-xl flex items-center justify-center">
                        <svg class="w-5 h-5 text-blue-400" fill="currentColor" viewBox="0 0 20 20"><path fill-rule="evenodd" d="M18 10a8 8 0 11-16 0 8 8 0 0116 0zm-7-4a1 1 0 11-2 0 1 1 0 012 0zM9 9a1 1 0 000 2v3a1 1 0 001 1h1a1 1 0 100-2v-3a1 1 0 00-1-1H9z" clip-rule="evenodd"/></svg>
                    </div>
                    <div>
                        <h3 class="font-semibold text-blue-400">You Already Own This Plugin</h3>
                        <p class="text-xs text-gray-400">Purchase additional licenses below if needed</p>
                    </div>
                </div>
            </div>
            <?php endif; ?>
            <div class="bg-gray-900 border border-gray-800 rounded-2xl p-6">
                <h3 class="text-lg font-semibold text-white mb-4">Choose Your License</h3>
                <div class="space-y-3">
                    <?php foreach ($pricings as $pricing): ?>
                    <div class="bg-gray-800/50 border border-gray-700 rounded-xl p-4 hover:border-brand-500/50 transition">
                        <div class="flex items-center justify-between mb-2">
                            <span class="font-medium text-white">
                                <?php
                                echo match($pricing['license_type']) {
                                    'monthly' => 'Monthly License',
                                    'yearly' => 'Yearly License',
                                    'lifetime' => 'Lifetime License',
                                    'custom_days' => $pricing['custom_days'] . ' Days License',
                                    default => ucfirst($pricing['license_type']),
                                };
                                ?>
                            </span>
                            <span class="text-xl font-bold text-brand-400"><?= format_price($pricing['price']) ?></span>
                        </div>
                        <p class="text-xs text-gray-500 mb-3">
                            <?php
                            echo match($pricing['license_type']) {
                                'monthly' => 'Billed monthly. Renew to keep access.',
                                'yearly' => 'Billed yearly. Best value for regular use.',
                                'lifetime' => 'One-time payment. Lifetime access.',
                                'custom_days' => 'Access for ' . $pricing['custom_days'] . ' days.',
                                default => '',
                            };
                            ?>
                        </p>
                        <?php if (is_logged_in()): ?>
                            <a href="/checkout/<?= $plugin['id'] ?>/<?= $pricing['id'] ?>" class="block w-full text-center px-4 py-2.5 bg-brand-600 hover:bg-brand-500 text-white text-sm font-medium rounded-lg transition">
                                Purchase Now
                            </a>
                        <?php else: ?>
                            <a href="/login" class="block w-full text-center px-4 py-2.5 bg-gray-700 hover:bg-gray-600 text-white text-sm font-medium rounded-lg transition">
                                Sign In to Purchase
                            </a>
                        <?php endif; ?>
                    </div>
                    <?php endforeach; ?>
                </div>
            </div>
            <?php endif; ?>
            <?php if (empty($pricings)): ?>
            <div class="bg-gray-900 border border-gray-800 rounded-2xl p-6 text-center">
                <p class="text-gray-400">Pricing coming soon.</p>
            </div>
            <?php endif; ?>

            <!-- Plugin Info Card -->
            <div class="bg-gray-900 border border-gray-800 rounded-2xl p-6">
                <h3 class="text-sm font-semibold text-white mb-4 uppercase tracking-wider">Plugin Info</h3>
                <dl class="space-y-3 text-sm">
                    <div class="flex justify-between">
                        <dt class="text-gray-400">Version</dt>
                        <dd class="text-white"><?= $latestVersion ? 'v' . e($latestVersion['version']) : '—' ?></dd>
                    </div>
                    <?php if ($latestVersion && $latestVersion['released_at']): ?>
                    <div class="flex justify-between">
                        <dt class="text-gray-400">Last Updated</dt>
                        <dd class="text-white"><?= format_date($latestVersion['released_at']) ?></dd>
                    </div>
                    <?php endif; ?>
                    <?php if ($plugin['category']): ?>
                    <div class="flex justify-between">
                        <dt class="text-gray-400">Category</dt>
                        <dd class="text-white"><?= e($plugin['category']) ?></dd>
                    </div>
                    <?php endif; ?>
                    <div class="flex justify-between">
                        <dt class="text-gray-400">Downloads</dt>
                        <dd class="text-white"><?= number_format($plugin['downloads_count']) ?></dd>
                    </div>
                    <div class="flex justify-between">
                        <dt class="text-gray-400">Added</dt>
                        <dd class="text-white"><?= format_date($plugin['created_at']) ?></dd>
                    </div>
                </dl>
            </div>
        </div>
    </div>
</div>

<?php require BASE_PATH . '/includes/footer.php'; ?>
