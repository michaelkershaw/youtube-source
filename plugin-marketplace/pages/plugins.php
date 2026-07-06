<?php
$pageTitle = 'Browse Plugins - ' . SITE_NAME;

$search = trim($_GET['search'] ?? '');
$category = trim($_GET['category'] ?? '');
$page = max(1, (int)($_GET['page'] ?? 1));

// Build query
$where = 'p.is_active = 1';
$params = [];

if ($search) {
    $where .= ' AND (p.name LIKE ? OR p.description LIKE ?)';
    $params[] = "%{$search}%";
    $params[] = "%{$search}%";
}
if ($category) {
    $where .= ' AND p.category = ?';
    $params[] = $category;
}

$total = Database::count('plugins p', $where, $params);
$pagination = paginate($total, ITEMS_PER_PAGE, $page);

$plugins = Database::fetchAll(
    "SELECT p.*, 
        (SELECT image_path FROM plugin_images WHERE plugin_id = p.id AND is_featured = 1 LIMIT 1) as featured_image,
        (SELECT version FROM plugin_versions WHERE plugin_id = p.id AND is_latest = 1 LIMIT 1) as latest_version,
        (SELECT MIN(price) FROM plugin_pricings WHERE plugin_id = p.id AND is_active = 1) as min_price
     FROM plugins p WHERE {$where} ORDER BY p.is_featured DESC, p.created_at DESC LIMIT ? OFFSET ?",
    array_merge($params, [$pagination['per_page'], $pagination['offset']])
);

// Get categories for filter
$categories = Database::fetchAll("SELECT DISTINCT category FROM plugins WHERE category IS NOT NULL AND category != '' AND is_active = 1 ORDER BY category");

require BASE_PATH . '/includes/header.php';
?>

<div class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-12">
    <!-- Page Header -->
    <div class="flex flex-col md:flex-row md:items-end md:justify-between gap-6 mb-10">
        <div>
            <h1 class="text-3xl sm:text-4xl font-bold text-white">Browse Plugins</h1>
            <p class="text-gray-400 mt-2"><?= $total ?> plugin<?= $total !== 1 ? 's' : '' ?> available</p>
        </div>
        
        <!-- Search & Filter -->
        <div class="flex flex-col sm:flex-row gap-3">
            <form method="GET" action="/plugins" class="flex gap-3">
                <div class="relative">
                    <svg class="absolute left-3 top-1/2 -translate-y-1/2 w-4 h-4 text-gray-500" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M21 21l-6-6m2-5a7 7 0 11-14 0 7 7 0 0114 0z"/></svg>
                    <input type="text" name="search" value="<?= e($search) ?>" placeholder="Search plugins..." 
                           class="w-full sm:w-64 pl-10 pr-4 py-2.5 bg-gray-900 border border-gray-700 rounded-xl text-sm text-white placeholder-gray-500 focus:outline-none focus:border-brand-500 focus:ring-1 focus:ring-brand-500 transition">
                </div>
                <?php if (!empty($categories)): ?>
                <select name="category" onchange="this.form.submit()" class="px-4 py-2.5 bg-gray-900 border border-gray-700 rounded-xl text-sm text-gray-300 focus:outline-none focus:border-brand-500 transition">
                    <option value="">All Categories</option>
                    <?php foreach ($categories as $cat): ?>
                        <option value="<?= e($cat['category']) ?>" <?= $category === $cat['category'] ? 'selected' : '' ?>><?= e($cat['category']) ?></option>
                    <?php endforeach; ?>
                </select>
                <?php endif; ?>
                <button type="submit" class="px-5 py-2.5 bg-brand-600 hover:bg-brand-500 text-white text-sm font-medium rounded-xl transition">Search</button>
            </form>
        </div>
    </div>

    <?php if (empty($plugins)): ?>
    <!-- Empty State -->
    <div class="text-center py-20">
        <svg class="w-16 h-16 text-gray-700 mx-auto mb-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="1" d="M20 7l-8-4-8 4m16 0l-8 4m8-4v10l-8 4m0-10L4 7m8 4v10M4 7v10l8 4"/></svg>
        <h3 class="text-lg font-medium text-gray-400">No plugins found</h3>
        <p class="text-gray-500 mt-1">Try adjusting your search or filters.</p>
    </div>
    <?php else: ?>
    <!-- Plugin Grid -->
    <div class="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-6">
        <?php foreach ($plugins as $plugin): ?>
        <a href="/plugin/<?= e($plugin['slug']) ?>" class="group bg-gray-900 border border-gray-800 rounded-2xl overflow-hidden hover:border-brand-500/50 transition-all hover:shadow-xl hover:shadow-brand-500/5">
            <div class="aspect-video bg-gray-800 relative overflow-hidden">
                <?php if ($plugin['featured_image']): ?>
                    <img src="/uploads/images/<?= e($plugin['featured_image']) ?>" alt="<?= e($plugin['name']) ?>" class="w-full h-full object-cover group-hover:scale-105 transition-transform duration-500">
                <?php else: ?>
                    <div class="w-full h-full flex items-center justify-center bg-gradient-to-br from-brand-600/20 to-purple-600/20">
                        <svg class="w-12 h-12 text-gray-600" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="1" d="M20 7l-8-4-8 4m16 0l-8 4m8-4v10l-8 4m0-10L4 7m8 4v10M4 7v10l8 4"/></svg>
                    </div>
                <?php endif; ?>
                <?php if ($plugin['is_featured']): ?>
                    <div class="absolute top-3 left-3"><span class="px-2.5 py-1 bg-amber-500/90 text-white text-xs font-bold rounded-lg">FEATURED</span></div>
                <?php endif; ?>
            </div>
            <div class="p-4">
                <h3 class="font-semibold text-white group-hover:text-brand-400 transition"><?= e($plugin['name']) ?></h3>
                <p class="text-xs text-gray-500 mt-1 line-clamp-2"><?= e($plugin['description']) ?></p>
                <?php if ($plugin['category']): ?>
                    <span class="inline-block mt-2 px-2 py-0.5 bg-gray-800 text-gray-400 text-xs rounded-md"><?= e($plugin['category']) ?></span>
                <?php endif; ?>
                <div class="flex items-center justify-between mt-3 pt-3 border-t border-gray-800">
                    <div class="flex items-center gap-2 text-xs text-gray-500">
                        <?php if ($plugin['latest_version']): ?>
                            <span>v<?= e($plugin['latest_version']) ?></span>
                        <?php endif; ?>
                        <span>&middot;</span>
                        <span><?= number_format($plugin['downloads_count']) ?> downloads</span>
                    </div>
                    <?php if ($plugin['min_price']): ?>
                        <span class="text-brand-400 font-semibold text-sm"><?= format_price($plugin['min_price']) ?></span>
                    <?php else: ?>
                        <span class="text-emerald-400 font-semibold text-sm">Free</span>
                    <?php endif; ?>
                </div>
            </div>
        </a>
        <?php endforeach; ?>
    </div>

    <!-- Pagination -->
    <?= render_pagination($pagination, '/plugins') ?>
    <?php endif; ?>
</div>

<?php require BASE_PATH . '/includes/footer.php'; ?>
