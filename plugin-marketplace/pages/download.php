<?php
require_login();

$pluginId = (int)($_GET['plugin_id'] ?? 0);
$versionId = (int)($_GET['version_id'] ?? 0);
$user = current_user();

$plugin = Database::fetch("SELECT * FROM plugins WHERE id = ?", [$pluginId]);
$version = Database::fetch("SELECT * FROM plugin_versions WHERE id = ? AND plugin_id = ?", [$versionId, $pluginId]);

if (!$plugin || !$version) {
    set_flash('error', 'Plugin or version not found.');
    redirect('/plugins');
}

// Check active license
$license = Database::fetch(
    "SELECT * FROM licenses WHERE user_id = ? AND plugin_id = ? AND status = 'active' AND (expires_at IS NULL OR expires_at > NOW())",
    [$user['id'], $plugin['id']]
);

if (!$license) {
    set_flash('error', 'You do not have an active license for this plugin.');
    redirect('/plugin/' . $plugin['slug']);
}

// Log download
$token = generate_download_token();
Database::insert('downloads', [
    'user_id' => $user['id'],
    'plugin_id' => $plugin['id'],
    'plugin_version_id' => $version['id'],
    'license_id' => $license['id'],
    'download_token' => $token,
    'ip_address' => $_SERVER['REMOTE_ADDR'] ?? '',
    'user_agent' => $_SERVER['HTTP_USER_AGENT'] ?? '',
    'downloaded_at' => date('Y-m-d H:i:s'),
    'created_at' => date('Y-m-d H:i:s'),
]);

// Increment download count
Database::query("UPDATE plugins SET downloads_count = downloads_count + 1 WHERE id = ?", [$plugin['id']]);

// Serve the file
$filePath = PLUGIN_FILES_PATH . '/' . $version['file_path'];

if ($version['file_path'] && file_exists($filePath)) {
    header('Content-Type: application/octet-stream');
    header('Content-Disposition: attachment; filename="' . ($version['file_name'] ?: basename($version['file_path'])) . '"');
    header('Content-Length: ' . filesize($filePath));
    header('Cache-Control: no-cache, must-revalidate');
    header('Pragma: no-cache');
    readfile($filePath);
    exit;
} else {
    // File not found on disk - show message
    set_flash('success', 'Download recorded for ' . $plugin['name'] . ' v' . $version['version'] . '. (File delivery will be available when plugin files are uploaded.)');
    redirect('/plugin/' . $plugin['slug']);
}
