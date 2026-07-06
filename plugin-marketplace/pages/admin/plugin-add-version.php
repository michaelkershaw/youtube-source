<?php
$pageTitle = 'Add Version - ' . SITE_NAME;
$id = (int)($_GET['id'] ?? 0);
$plugin = Database::fetch("SELECT * FROM plugins WHERE id = ?", [$id]);
if (!$plugin) { set_flash('error', 'Plugin not found.'); redirect('/admin/plugins'); }

if ($method === 'POST') {
    require_csrf();
    $version = trim($_POST['version'] ?? '');
    $changelog = trim($_POST['changelog'] ?? '');

    if (empty($version)) {
        set_flash('error', 'Version number is required.');
        redirect('/admin/plugins/' . $id . '/add-version');
    }

    if (Database::count('plugin_versions', 'plugin_id = ? AND version = ?', [$id, $version]) > 0) {
        set_flash('error', 'Version ' . $version . ' already exists.');
        redirect('/admin/plugins/' . $id . '/add-version');
    }

    $filePath = '';
    $fileName = '';
    $fileSize = 0;
    $fileHash = '';

    if (!empty($_FILES['plugin_file']) && $_FILES['plugin_file']['error'] === UPLOAD_ERR_OK) {
        $pluginDir = PLUGIN_FILES_PATH . '/' . $plugin['slug'];
        $uploadedFile = upload_file($_FILES['plugin_file'], $pluginDir, ALLOWED_PLUGIN_EXTENSIONS, MAX_PLUGIN_FILE_SIZE);
        if ($uploadedFile) {
            $filePath = $plugin['slug'] . '/' . $uploadedFile;
            $fileName = $_FILES['plugin_file']['name'];
            $fileSize = $_FILES['plugin_file']['size'];
            $fileHash = hash_file('sha256', $pluginDir . '/' . $uploadedFile);
        }
    }

    // Unset previous latest
    Database::update('plugin_versions', ['is_latest' => 0], 'plugin_id = ?', [$id]);

    Database::insert('plugin_versions', [
        'plugin_id' => $id,
        'version' => $version,
        'changelog' => $changelog ?: null,
        'file_path' => $filePath,
        'file_name' => $fileName,
        'file_size' => $fileSize,
        'file_hash' => $fileHash,
        'is_latest' => 1,
        'is_active' => 1,
        'released_at' => date('Y-m-d H:i:s'),
        'created_at' => date('Y-m-d H:i:s'),
    ]);

    set_flash('success', 'Version ' . $version . ' added successfully.');
    redirect('/admin/plugins/' . $id);
}

require BASE_PATH . '/includes/admin-header.php';
?>

<div class="mb-6">
    <a href="/admin/plugins/<?= $id ?>" class="text-sm text-gray-400 hover:text-white transition flex items-center gap-1 mb-2">
        <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M10 19l-7-7m0 0l7-7m-7 7h18"/></svg> Back to <?= e($plugin['name']) ?>
    </a>
    <h1 class="text-2xl font-bold text-white">Add Version: <?= e($plugin['name']) ?></h1>
</div>

<form method="POST" action="/admin/plugins/<?= $id ?>/add-version" enctype="multipart/form-data" class="space-y-6 max-w-2xl">
    <?= csrf_field() ?>
    <div class="bg-gray-900 border border-gray-800 rounded-2xl p-6">
        <div class="space-y-4">
            <div>
                <label class="block text-sm font-medium text-gray-300 mb-1.5">Version Number *</label>
                <input type="text" name="version" required placeholder="e.g. 2.0.0" class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition">
            </div>
            <div>
                <label class="block text-sm font-medium text-gray-300 mb-1.5">Plugin File</label>
                <input type="file" name="plugin_file" class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition file:mr-4 file:py-1 file:px-3 file:rounded-lg file:border-0 file:text-sm file:bg-brand-600 file:text-white">
                <p class="text-xs text-gray-500 mt-1">Allowed: <?= implode(', ', ALLOWED_PLUGIN_EXTENSIONS) ?></p>
            </div>
            <div>
                <label class="block text-sm font-medium text-gray-300 mb-1.5">Changelog / Release Notes</label>
                <textarea name="changelog" rows="5" class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition" placeholder="What's new in this version..."></textarea>
            </div>
        </div>
    </div>
    <div class="flex items-center gap-4">
        <button type="submit" class="px-8 py-3 bg-brand-600 hover:bg-brand-500 text-white font-semibold rounded-xl transition">Add Version</button>
        <a href="/admin/plugins/<?= $id ?>" class="px-8 py-3 bg-gray-800 hover:bg-gray-700 text-gray-300 font-medium rounded-xl transition">Cancel</a>
    </div>
</form>

<?php require BASE_PATH . '/includes/admin-footer.php'; ?>
