<?php
require_admin();
$id = (int)($_GET['id'] ?? 0);
$plugin = Database::fetch("SELECT * FROM plugins WHERE id = ?", [$id]);
if (!$plugin) { set_flash('error', 'Plugin not found.'); redirect('/admin/plugins'); }

// Delete associated images from disk
$images = Database::fetchAll("SELECT * FROM plugin_images WHERE plugin_id = ?", [$id]);
foreach ($images as $img) {
    delete_file(PLUGIN_IMAGES_PATH . '/' . $img['image_path']);
}

// Delete associated version files from disk
$versions = Database::fetchAll("SELECT * FROM plugin_versions WHERE plugin_id = ?", [$id]);
foreach ($versions as $ver) {
    if ($ver['file_path']) {
        delete_file(PLUGIN_FILES_PATH . '/' . $ver['file_path']);
    }
}

Database::delete('plugins', 'id = ?', [$id]);
set_flash('success', 'Plugin "' . $plugin['name'] . '" deleted successfully.');
redirect('/admin/plugins');
