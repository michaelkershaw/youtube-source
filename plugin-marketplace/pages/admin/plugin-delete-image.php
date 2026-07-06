<?php
require_admin();
$id = (int)($_GET['id'] ?? 0);
$pluginId = (int)($_GET['plugin_id'] ?? 0);

$image = Database::fetch("SELECT * FROM plugin_images WHERE id = ?", [$id]);
if (!$image) { set_flash('error', 'Image not found.'); redirect('/admin/plugins'); }

delete_file(PLUGIN_IMAGES_PATH . '/' . $image['image_path']);
Database::delete('plugin_images', 'id = ?', [$id]);

set_flash('success', 'Image deleted.');
redirect('/admin/plugins/' . ($pluginId ?: $image['plugin_id']));
