<?php
$pageTitle = 'Add Plugin - ' . SITE_NAME;
$errors = get_validation_errors();

if ($method === 'POST') {
    // If payload exceeds server post_max_size, PHP empties $_POST/$_FILES and CSRF check fails.
    // Detect that case early and show a useful error instead of "Invalid security token".
    if (empty($_POST) && empty($_FILES) && !empty($_SERVER['CONTENT_LENGTH'])) {
        set_flash('error', 'Upload is too large for server limits. Create the plugin first without files, then add version/images separately or increase PHP upload limits.');
        redirect('/admin/plugins/create');
    }

    require_csrf();
    $name = trim($_POST['name'] ?? '');
    $identifier = trim($_POST['identifier'] ?? '');
    $description = trim($_POST['description'] ?? '');
    $fullDescription = trim($_POST['full_description'] ?? '');
    $category = trim($_POST['category'] ?? '');
    $isActive = isset($_POST['is_active']) ? 1 : 0;
    $isFeatured = isset($_POST['is_featured']) ? 1 : 0;

    $errs = [];
    if (empty($name)) $errs['name'] = 'Plugin name is required.';
    if (empty($identifier)) $errs['identifier'] = 'Identifier is required.';
    if (empty($description)) $errs['description'] = 'Description is required.';

    $slug = generate_slug($name);
    if (!empty($name) && Database::count('plugins', 'slug = ?', [$slug]) > 0) {
        $errs['name'] = 'A plugin with this name already exists.';
    }
    if (!empty($identifier) && Database::count('plugins', 'identifier = ?', [$identifier]) > 0) {
        $errs['identifier'] = 'This identifier is already in use.';
    }

    if (!empty($errs)) {
        set_validation_errors($errs);
        set_old_input($_POST);
        redirect('/admin/plugins/create');
    }

    $pluginId = Database::insert('plugins', [
        'name' => $name,
        'slug' => $slug,
        'identifier' => $identifier,
        'description' => $description,
        'full_description' => $fullDescription ?: null,
        'category' => $category ?: null,
        'is_active' => $isActive,
        'is_featured' => $isFeatured,
        'created_at' => date('Y-m-d H:i:s'),
        'updated_at' => date('Y-m-d H:i:s'),
    ]);

    // Pricing
    foreach (['monthly', 'yearly', 'lifetime'] as $type) {
        $priceField = 'price_' . $type;
        if (!empty($_POST[$priceField]) && is_numeric($_POST[$priceField])) {
            Database::insert('plugin_pricings', [
                'plugin_id' => $pluginId,
                'license_type' => $type,
                'price' => (float)$_POST[$priceField],
                'is_active' => 1,
                'created_at' => date('Y-m-d H:i:s'),
                'updated_at' => date('Y-m-d H:i:s'),
            ]);
        }
    }
    // Custom days pricing
    if (!empty($_POST['price_custom']) && is_numeric($_POST['price_custom']) && !empty($_POST['custom_days']) && is_numeric($_POST['custom_days'])) {
        Database::insert('plugin_pricings', [
            'plugin_id' => $pluginId,
            'license_type' => 'custom_days',
            'custom_days' => (int)$_POST['custom_days'],
            'price' => (float)$_POST['price_custom'],
            'is_active' => 1,
            'created_at' => date('Y-m-d H:i:s'),
            'updated_at' => date('Y-m-d H:i:s'),
        ]);
    }

    // License format
    $licensePrefix = trim($_POST['license_prefix'] ?? '');
    $licenseFormat = trim($_POST['license_format'] ?? '');
    if ($licensePrefix || $licenseFormat) {
        Database::query(
            "INSERT INTO license_formats (plugin_id, prefix, format_pattern, `separator`, is_active, created_at, updated_at) 
             VALUES (?, ?, ?, '-', 1, ?, ?)",
            [
                $pluginId,
                $licensePrefix ?: null,
                $licenseFormat ?: 'XXXX-XXXX-XXXX-XXXX',
                date('Y-m-d H:i:s'),
                date('Y-m-d H:i:s')
            ]
        );
    }

    // Version upload
    $version = trim($_POST['version'] ?? '');
    if ($version) {
        $filePath = '';
        $fileName = '';
        $fileSize = 0;
        $fileHash = '';

        if (!empty($_FILES['plugin_file']) && $_FILES['plugin_file']['error'] === UPLOAD_ERR_OK) {
            $pluginDir = PLUGIN_FILES_PATH . '/' . $slug;
            $uploadedFile = upload_file($_FILES['plugin_file'], $pluginDir, ALLOWED_PLUGIN_EXTENSIONS, MAX_PLUGIN_FILE_SIZE);
            if ($uploadedFile) {
                $filePath = $slug . '/' . $uploadedFile;
                $fileName = $_FILES['plugin_file']['name'];
                $fileSize = $_FILES['plugin_file']['size'];
                $fileHash = hash_file('sha256', $pluginDir . '/' . $uploadedFile);
            }
        }

        Database::insert('plugin_versions', [
            'plugin_id' => $pluginId,
            'version' => $version,
            'changelog' => trim($_POST['changelog'] ?? '') ?: null,
            'file_path' => $filePath,
            'file_name' => $fileName,
            'file_size' => $fileSize,
            'file_hash' => $fileHash,
            'is_latest' => 1,
            'is_active' => 1,
            'released_at' => date('Y-m-d H:i:s'),
            'created_at' => date('Y-m-d H:i:s'),
        ]);
    }

    // Image uploads
    if (!empty($_FILES['images'])) {
        $imgDir = PLUGIN_IMAGES_PATH;
        $fileCount = count($_FILES['images']['name']);
        for ($i = 0; $i < $fileCount; $i++) {
            if ($_FILES['images']['error'][$i] === UPLOAD_ERR_OK) {
                $file = [
                    'name' => $_FILES['images']['name'][$i],
                    'tmp_name' => $_FILES['images']['tmp_name'][$i],
                    'error' => $_FILES['images']['error'][$i],
                    'size' => $_FILES['images']['size'][$i],
                ];
                $uploaded = upload_file($file, $imgDir, ALLOWED_IMAGE_EXTENSIONS, MAX_IMAGE_FILE_SIZE);
                if ($uploaded) {
                    Database::insert('plugin_images', [
                        'plugin_id' => $pluginId,
                        'image_path' => $uploaded,
                        'image_name' => $_FILES['images']['name'][$i],
                        'sort_order' => $i,
                        'is_featured' => ($i === 0) ? 1 : 0,
                        'created_at' => date('Y-m-d H:i:s'),
                    ]);
                }
            }
        }
    }

    set_flash('success', 'Plugin created successfully.');
    redirect('/admin/plugins');
}

require BASE_PATH . '/includes/admin-header.php';
?>

<div class="mb-6">
    <a href="/admin/plugins" class="text-sm text-gray-400 hover:text-white transition flex items-center gap-1 mb-2">
        <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M10 19l-7-7m0 0l7-7m-7 7h18"/></svg> Back to Plugins
    </a>
    <h1 class="text-2xl font-bold text-white">Add New Plugin</h1>
</div>

<form method="POST" action="/admin/plugins/create" enctype="multipart/form-data" class="space-y-6 max-w-4xl">
    <?= csrf_field() ?>

    <!-- Basic Info -->
    <div class="bg-gray-900 border border-gray-800 rounded-2xl p-6">
        <h2 class="text-lg font-semibold text-white mb-4">Basic Information</h2>
        <div class="grid grid-cols-1 md:grid-cols-2 gap-4">
            <div class="md:col-span-2">
                <label class="block text-sm font-medium text-gray-300 mb-1.5">Plugin Name *</label>
                <input type="text" name="name" value="<?= old('name') ?>" required class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition">
                <?php if (!empty($errors['name'])): ?><p class="mt-1 text-sm text-red-400"><?= e($errors['name']) ?></p><?php endif; ?>
            </div>
            <div class="md:col-span-2">
                <label class="block text-sm font-medium text-gray-300 mb-1.5">Identifier *</label>
                <input type="text" name="identifier" value="<?= old('identifier') ?>" required class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition" placeholder="e.g. tellymedia-reborn">
                <p class="mt-1 text-xs text-gray-500">Unique identifier used by the licensing API. Use lowercase letters, numbers, and hyphens only.</p>
                <?php if (!empty($errors['identifier'])): ?><p class="mt-1 text-sm text-red-400"><?= e($errors['identifier']) ?></p><?php endif; ?>
            </div>
            <div class="md:col-span-2">
                <label class="block text-sm font-medium text-gray-300 mb-1.5">Short Description *</label>
                <textarea name="description" rows="2" required class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition"><?= old('description') ?></textarea>
                <?php if (!empty($errors['description'])): ?><p class="mt-1 text-sm text-red-400"><?= e($errors['description']) ?></p><?php endif; ?>
            </div>
            <div class="md:col-span-2">
                <label class="block text-sm font-medium text-gray-300 mb-1.5">Full Description</label>
                <textarea name="full_description" rows="5" class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition"><?= old('full_description') ?></textarea>
            </div>
            <div>
                <label class="block text-sm font-medium text-gray-300 mb-1.5">Category</label>
                <input type="text" name="category" value="<?= old('category') ?>" class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition" placeholder="e.g. Effects, Skins, Tools">
            </div>
            <div class="flex items-center gap-6 pt-6">
                <label class="flex items-center gap-2 cursor-pointer">
                    <input type="checkbox" name="is_active" value="1" checked class="w-4 h-4 rounded border-gray-600 bg-gray-800 text-brand-600 focus:ring-brand-500">
                    <span class="text-sm text-gray-300">Active</span>
                </label>
                <label class="flex items-center gap-2 cursor-pointer">
                    <input type="checkbox" name="is_featured" value="1" class="w-4 h-4 rounded border-gray-600 bg-gray-800 text-brand-600 focus:ring-brand-500">
                    <span class="text-sm text-gray-300">Featured</span>
                </label>
            </div>
        </div>
    </div>

    <!-- Pricing -->
    <div class="bg-gray-900 border border-gray-800 rounded-2xl p-6">
        <h2 class="text-lg font-semibold text-white mb-4">Pricing Options</h2>
        <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4">
            <div>
                <label class="block text-sm font-medium text-gray-300 mb-1.5">Monthly Price ($)</label>
                <input type="number" name="price_monthly" step="0.01" min="0" value="<?= old('price_monthly') ?>" class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition">
            </div>
            <div>
                <label class="block text-sm font-medium text-gray-300 mb-1.5">Yearly Price ($)</label>
                <input type="number" name="price_yearly" step="0.01" min="0" value="<?= old('price_yearly') ?>" class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition">
            </div>
            <div>
                <label class="block text-sm font-medium text-gray-300 mb-1.5">Lifetime Price ($)</label>
                <input type="number" name="price_lifetime" step="0.01" min="0" value="<?= old('price_lifetime') ?>" class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition">
            </div>
            <div>
                <label class="block text-sm font-medium text-gray-300 mb-1.5">Custom Days Price ($)</label>
                <input type="number" name="price_custom" step="0.01" min="0" value="<?= old('price_custom') ?>" class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition">
                <input type="number" name="custom_days" min="1" value="<?= old('custom_days') ?>" placeholder="Number of days" class="w-full mt-2 px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition">
            </div>
        </div>
    </div>

    <!-- License Format -->
    <div class="bg-gray-900 border border-gray-800 rounded-2xl p-6">
        <h2 class="text-lg font-semibold text-white mb-4">License Key Format</h2>
        <p class="text-xs text-gray-500 mb-4">Leave blank to use default format (XXXX-XXXX-XXXX-XXXX). Use X for alphanumeric, # for digit, A for letter.</p>
        <div class="grid grid-cols-1 md:grid-cols-2 gap-4">
            <div>
                <label class="block text-sm font-medium text-gray-300 mb-1.5">Prefix (e.g. VDJ, PLUGINB)</label>
                <input type="text" name="license_prefix" value="<?= old('license_prefix') ?>" class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition">
            </div>
            <div>
                <label class="block text-sm font-medium text-gray-300 mb-1.5">Format Pattern (e.g. XXXX-XXXX-XXXX)</label>
                <input type="text" name="license_format" value="<?= old('license_format') ?>" class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition">
            </div>
        </div>
    </div>

    <!-- Initial Version -->
    <div class="bg-gray-900 border border-gray-800 rounded-2xl p-6">
        <h2 class="text-lg font-semibold text-white mb-4">Initial Version (Optional)</h2>
        <div class="grid grid-cols-1 md:grid-cols-2 gap-4">
            <div>
                <label class="block text-sm font-medium text-gray-300 mb-1.5">Version Number</label>
                <input type="text" name="version" value="<?= old('version') ?>" class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition" placeholder="e.g. 1.0.0">
            </div>
            <div>
                <label class="block text-sm font-medium text-gray-300 mb-1.5">Plugin File</label>
                <input type="file" name="plugin_file" class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition file:mr-4 file:py-1 file:px-3 file:rounded-lg file:border-0 file:text-sm file:bg-brand-600 file:text-white hover:file:bg-brand-500">
            </div>
            <div class="md:col-span-2">
                <label class="block text-sm font-medium text-gray-300 mb-1.5">Changelog</label>
                <textarea name="changelog" rows="3" class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition"><?= old('changelog') ?></textarea>
            </div>
        </div>
    </div>

    <!-- Images -->
    <div class="bg-gray-900 border border-gray-800 rounded-2xl p-6">
        <h2 class="text-lg font-semibold text-white mb-4">Plugin Images</h2>
        <p class="text-xs text-gray-500 mb-4">First image will be set as featured. Max 5MB per image.</p>
        <input type="file" name="images[]" multiple accept="image/*" class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition file:mr-4 file:py-1 file:px-3 file:rounded-lg file:border-0 file:text-sm file:bg-brand-600 file:text-white hover:file:bg-brand-500">
    </div>

    <div class="flex items-center gap-4">
        <button type="submit" class="px-8 py-3 bg-brand-600 hover:bg-brand-500 text-white font-semibold rounded-xl transition shadow-lg shadow-brand-600/20">Create Plugin</button>
        <a href="/admin/plugins" class="px-8 py-3 bg-gray-800 hover:bg-gray-700 text-gray-300 font-medium rounded-xl transition">Cancel</a>
    </div>
</form>

<?php
clear_old_input();
require BASE_PATH . '/includes/admin-footer.php';
?>
