<?php
$pageTitle = 'Add Image - ' . SITE_NAME;
$id = (int)($_GET['id'] ?? 0);
$plugin = Database::fetch("SELECT * FROM plugins WHERE id = ?", [$id]);
if (!$plugin) { set_flash('error', 'Plugin not found.'); redirect('/admin/plugins'); }

if ($method === 'POST') {
    require_csrf();
    $caption = trim($_POST['caption'] ?? '');
    $isFeatured = isset($_POST['is_featured']) ? 1 : 0;

    if (!empty($_FILES['image']) && $_FILES['image']['error'] === UPLOAD_ERR_OK) {
        $uploaded = upload_file($_FILES['image'], PLUGIN_IMAGES_PATH, ALLOWED_IMAGE_EXTENSIONS, MAX_IMAGE_FILE_SIZE);
        if ($uploaded) {
            if ($isFeatured) {
                Database::update('plugin_images', ['is_featured' => 0], 'plugin_id = ?', [$id]);
            }
            $maxSort = Database::fetch("SELECT COALESCE(MAX(sort_order), 0) as mx FROM plugin_images WHERE plugin_id = ?", [$id])['mx'];
            Database::insert('plugin_images', [
                'plugin_id' => $id,
                'image_path' => $uploaded,
                'image_name' => $_FILES['image']['name'],
                'caption' => $caption ?: null,
                'sort_order' => $maxSort + 1,
                'is_featured' => $isFeatured,
                'created_at' => date('Y-m-d H:i:s'),
            ]);
            set_flash('success', 'Image uploaded successfully.');
        } else {
            set_flash('error', 'Failed to upload image. Check file type and size.');
        }
    } else {
        set_flash('error', 'Please select an image file.');
    }
    redirect('/admin/plugins/' . $id);
}

require BASE_PATH . '/includes/admin-header.php';
?>

<div class="mb-6">
    <a href="/admin/plugins/<?= $id ?>" class="text-sm text-gray-400 hover:text-white transition flex items-center gap-1 mb-2">
        <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M10 19l-7-7m0 0l7-7m-7 7h18"/></svg> Back to <?= e($plugin['name']) ?>
    </a>
    <h1 class="text-2xl font-bold text-white">Add Image: <?= e($plugin['name']) ?></h1>
</div>

<form method="POST" action="/admin/plugins/<?= $id ?>/add-image" enctype="multipart/form-data" class="space-y-6 max-w-2xl">
    <?= csrf_field() ?>
    <div class="bg-gray-900 border border-gray-800 rounded-2xl p-6 space-y-4">
        <div>
            <label class="block text-sm font-medium text-gray-300 mb-1.5">Image File *</label>
            <input type="file" name="image" required accept="image/*" class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white file:mr-4 file:py-1 file:px-3 file:rounded-lg file:border-0 file:text-sm file:bg-brand-600 file:text-white">
            <p class="text-xs text-gray-500 mt-1">Max 5MB. Allowed: <?= implode(', ', ALLOWED_IMAGE_EXTENSIONS) ?></p>
        </div>
        <div>
            <label class="block text-sm font-medium text-gray-300 mb-1.5">Caption</label>
            <input type="text" name="caption" class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition">
        </div>
        <label class="flex items-center gap-2 cursor-pointer">
            <input type="checkbox" name="is_featured" value="1" class="w-4 h-4 rounded border-gray-600 bg-gray-800 text-brand-600 focus:ring-brand-500">
            <span class="text-sm text-gray-300">Set as featured image</span>
        </label>
    </div>
    <div class="flex items-center gap-4">
        <button type="submit" class="px-8 py-3 bg-brand-600 hover:bg-brand-500 text-white font-semibold rounded-xl transition">Upload Image</button>
        <a href="/admin/plugins/<?= $id ?>" class="px-8 py-3 bg-gray-800 hover:bg-gray-700 text-gray-300 font-medium rounded-xl transition">Cancel</a>
    </div>
</form>

<?php require BASE_PATH . '/includes/admin-footer.php'; ?>
