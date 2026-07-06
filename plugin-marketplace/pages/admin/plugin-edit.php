<?php
$pageTitle = 'Edit Plugin - ' . SITE_NAME;
$id = (int)($_GET['id'] ?? 0);
$plugin = Database::fetch("SELECT * FROM plugins WHERE id = ?", [$id]);
if (!$plugin) { set_flash('error', 'Plugin not found.'); redirect('/admin/plugins'); }

$errors = get_validation_errors();
$pricings = Database::fetchAll("SELECT * FROM plugin_pricings WHERE plugin_id = ?", [$id]);
$licenseFormat = Database::fetch("SELECT * FROM license_formats WHERE plugin_id = ?", [$id]);

// Index pricings by type
$priceMap = [];
foreach ($pricings as $pr) {
    $key = $pr['license_type'] === 'custom_days' ? 'custom' : $pr['license_type'];
    $priceMap[$key] = $pr;
}

if ($method === 'POST') {
    require_csrf();
    $name = trim($_POST['name'] ?? '');
    $description = trim($_POST['description'] ?? '');
    $fullDescription = trim($_POST['full_description'] ?? '');
    $category = trim($_POST['category'] ?? '');
    $isActive = isset($_POST['is_active']) ? 1 : 0;
    $isFeatured = isset($_POST['is_featured']) ? 1 : 0;

    $errs = [];
    if (empty($name)) $errs['name'] = 'Plugin name is required.';
    if (empty($description)) $errs['description'] = 'Description is required.';

    $slug = generate_slug($name);
    if (!empty($name) && Database::count('plugins', 'slug = ? AND id != ?', [$slug, $id]) > 0) {
        $errs['name'] = 'A plugin with this name already exists.';
    }

    if (!empty($errs)) {
        set_validation_errors($errs);
        redirect('/admin/plugins/edit/' . $id);
    }

    Database::update('plugins', [
        'name' => $name,
        'slug' => $slug,
        'description' => $description,
        'full_description' => $fullDescription ?: null,
        'category' => $category ?: null,
        'is_active' => $isActive,
        'is_featured' => $isFeatured,
        'updated_at' => date('Y-m-d H:i:s'),
    ], 'id = ?', [$id]);

    // Update pricing
    foreach (['monthly', 'yearly', 'lifetime'] as $type) {
        $priceField = 'price_' . $type;
        $existing = Database::fetch("SELECT id FROM plugin_pricings WHERE plugin_id = ? AND license_type = ?", [$id, $type]);
        if (!empty($_POST[$priceField]) && is_numeric($_POST[$priceField])) {
            if ($existing) {
                Database::update('plugin_pricings', ['price' => (float)$_POST[$priceField]], 'id = ?', [$existing['id']]);
            } else {
                Database::insert('plugin_pricings', [
                    'plugin_id' => $id, 'license_type' => $type,
                    'price' => (float)$_POST[$priceField], 'is_active' => 1,
                    'created_at' => date('Y-m-d H:i:s'), 'updated_at' => date('Y-m-d H:i:s'),
                ]);
            }
        } elseif ($existing) {
            Database::delete('plugin_pricings', 'id = ?', [$existing['id']]);
        }
    }

    // Custom days
    $existingCustom = Database::fetch("SELECT id FROM plugin_pricings WHERE plugin_id = ? AND license_type = 'custom_days'", [$id]);
    if (!empty($_POST['price_custom']) && is_numeric($_POST['price_custom']) && !empty($_POST['custom_days'])) {
        if ($existingCustom) {
            Database::update('plugin_pricings', ['price' => (float)$_POST['price_custom'], 'custom_days' => (int)$_POST['custom_days']], 'id = ?', [$existingCustom['id']]);
        } else {
            Database::insert('plugin_pricings', [
                'plugin_id' => $id, 'license_type' => 'custom_days',
                'custom_days' => (int)$_POST['custom_days'], 'price' => (float)$_POST['price_custom'],
                'is_active' => 1, 'created_at' => date('Y-m-d H:i:s'), 'updated_at' => date('Y-m-d H:i:s'),
            ]);
        }
    } elseif ($existingCustom) {
        Database::delete('plugin_pricings', 'id = ?', [$existingCustom['id']]);
    }

    // License format
    $licPrefix = trim($_POST['license_prefix'] ?? '');
    $licFormat = trim($_POST['license_format'] ?? '');
    if ($licPrefix || $licFormat) {
        if ($licenseFormat) {
            Database::update('license_formats', [
                'prefix' => $licPrefix ?: null,
                'format_pattern' => $licFormat ?: 'XXXX-XXXX-XXXX-XXXX',
            ], 'id = ?', [$licenseFormat['id']]);
        } else {
            Database::insert('license_formats', [
                'plugin_id' => $id, 'prefix' => $licPrefix ?: null,
                'format_pattern' => $licFormat ?: 'XXXX-XXXX-XXXX-XXXX',
                'separator' => '-', 'is_active' => 1,
                'created_at' => date('Y-m-d H:i:s'), 'updated_at' => date('Y-m-d H:i:s'),
            ]);
        }
    }

    set_flash('success', 'Plugin updated successfully.');
    redirect('/admin/plugins');
}

require BASE_PATH . '/includes/admin-header.php';
?>

<div class="mb-6">
    <a href="/admin/plugins" class="text-sm text-gray-400 hover:text-white transition flex items-center gap-1 mb-2">
        <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M10 19l-7-7m0 0l7-7m-7 7h18"/></svg> Back to Plugins
    </a>
    <h1 class="text-2xl font-bold text-white">Edit: <?= e($plugin['name']) ?></h1>
</div>

<form method="POST" action="/admin/plugins/edit/<?= $id ?>" class="space-y-6 max-w-4xl">
    <?= csrf_field() ?>

    <div class="bg-gray-900 border border-gray-800 rounded-2xl p-6">
        <h2 class="text-lg font-semibold text-white mb-4">Basic Information</h2>
        <div class="grid grid-cols-1 md:grid-cols-2 gap-4">
            <div class="md:col-span-2">
                <label class="block text-sm font-medium text-gray-300 mb-1.5">Plugin Name *</label>
                <input type="text" name="name" value="<?= e($plugin['name']) ?>" required class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition">
                <?php if (!empty($errors['name'])): ?><p class="mt-1 text-sm text-red-400"><?= e($errors['name']) ?></p><?php endif; ?>
            </div>
            <div class="md:col-span-2">
                <label class="block text-sm font-medium text-gray-300 mb-1.5">Short Description *</label>
                <textarea name="description" rows="2" required class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition"><?= e($plugin['description']) ?></textarea>
            </div>
            <div class="md:col-span-2">
                <label class="block text-sm font-medium text-gray-300 mb-1.5">Full Description</label>
                <textarea name="full_description" rows="5" class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition"><?= e($plugin['full_description'] ?? '') ?></textarea>
            </div>
            <div>
                <label class="block text-sm font-medium text-gray-300 mb-1.5">Category</label>
                <input type="text" name="category" value="<?= e($plugin['category'] ?? '') ?>" class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition">
            </div>
            <div class="flex items-center gap-6 pt-6">
                <label class="flex items-center gap-2 cursor-pointer">
                    <input type="checkbox" name="is_active" value="1" <?= $plugin['is_active'] ? 'checked' : '' ?> class="w-4 h-4 rounded border-gray-600 bg-gray-800 text-brand-600 focus:ring-brand-500">
                    <span class="text-sm text-gray-300">Active</span>
                </label>
                <label class="flex items-center gap-2 cursor-pointer">
                    <input type="checkbox" name="is_featured" value="1" <?= $plugin['is_featured'] ? 'checked' : '' ?> class="w-4 h-4 rounded border-gray-600 bg-gray-800 text-brand-600 focus:ring-brand-500">
                    <span class="text-sm text-gray-300">Featured</span>
                </label>
            </div>
        </div>
    </div>

    <div class="bg-gray-900 border border-gray-800 rounded-2xl p-6">
        <h2 class="text-lg font-semibold text-white mb-4">Pricing Options</h2>
        <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4">
            <div>
                <label class="block text-sm font-medium text-gray-300 mb-1.5">Monthly Price ($)</label>
                <input type="number" name="price_monthly" step="0.01" min="0" value="<?= e($priceMap['monthly']['price'] ?? '') ?>" class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition">
            </div>
            <div>
                <label class="block text-sm font-medium text-gray-300 mb-1.5">Yearly Price ($)</label>
                <input type="number" name="price_yearly" step="0.01" min="0" value="<?= e($priceMap['yearly']['price'] ?? '') ?>" class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition">
            </div>
            <div>
                <label class="block text-sm font-medium text-gray-300 mb-1.5">Lifetime Price ($)</label>
                <input type="number" name="price_lifetime" step="0.01" min="0" value="<?= e($priceMap['lifetime']['price'] ?? '') ?>" class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition">
            </div>
            <div>
                <label class="block text-sm font-medium text-gray-300 mb-1.5">Custom Days Price ($)</label>
                <input type="number" name="price_custom" step="0.01" min="0" value="<?= e($priceMap['custom']['price'] ?? '') ?>" class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition">
                <input type="number" name="custom_days" min="1" value="<?= e($priceMap['custom']['custom_days'] ?? '') ?>" placeholder="Number of days" class="w-full mt-2 px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition">
            </div>
        </div>
    </div>

    <div class="bg-gray-900 border border-gray-800 rounded-2xl p-6">
        <h2 class="text-lg font-semibold text-white mb-4">License Key Format</h2>
        <div class="grid grid-cols-1 md:grid-cols-2 gap-4">
            <div>
                <label class="block text-sm font-medium text-gray-300 mb-1.5">Prefix</label>
                <input type="text" name="license_prefix" value="<?= e($licenseFormat['prefix'] ?? '') ?>" class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition">
            </div>
            <div>
                <label class="block text-sm font-medium text-gray-300 mb-1.5">Format Pattern</label>
                <input type="text" name="license_format" value="<?= e($licenseFormat['format_pattern'] ?? '') ?>" class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition">
            </div>
        </div>
    </div>

    <div class="flex items-center gap-4">
        <button type="submit" class="px-8 py-3 bg-brand-600 hover:bg-brand-500 text-white font-semibold rounded-xl transition">Update Plugin</button>
        <a href="/admin/plugins" class="px-8 py-3 bg-gray-800 hover:bg-gray-700 text-gray-300 font-medium rounded-xl transition">Cancel</a>
    </div>
</form>

<?php require BASE_PATH . '/includes/admin-footer.php'; ?>
