<?php
/**
 * Plugin Marketplace - Configuration
 * 
 * Dynamic multi-domain support: just update these values per deployment.
 */

// Session settings (must be before any output)
ini_set('session.cookie_httponly', '1');
ini_set('session.use_strict_mode', '1');

// Error reporting (disable in production)
error_reporting(E_ALL);
ini_set('display_errors', '1');

// Base path detection (works on any domain)
define('BASE_PATH', __DIR__);
define('UPLOADS_PATH', BASE_PATH . '/uploads');
define('PLUGIN_FILES_PATH', UPLOADS_PATH . '/plugins');
define('PLUGIN_IMAGES_PATH', UPLOADS_PATH . '/images');

// Detect base URL dynamically for multi-domain support
$protocol = (!empty($_SERVER['HTTPS']) && $_SERVER['HTTPS'] !== 'off') ? 'https' : 'http';
$host = $_SERVER['HTTP_HOST'] ?? 'localhost';
define('BASE_URL', $protocol . '://' . $host);
define('SITE_NAME', 'Plugin Marketplace');

// Database configuration
define('DB_HOST', 'localhost');
define('DB_PORT', '3306');
define('DB_NAME', 'djmickyk_mrk-vdj-plugins');
define('DB_USER', 'djmickyk_mrk-vdj-plugins');
define('DB_PASS', 'Adew4u2day@5');
define('DB_CHARSET', 'utf8mb4');

// Security
define('CSRF_TOKEN_NAME', 'csrf_token');
define('PASSWORD_MIN_LENGTH', 8);
define('DOWNLOAD_TOKEN_EXPIRY', 3600); // 1 hour in seconds
define('MAX_LOGIN_ATTEMPTS', 5);
define('LOGIN_LOCKOUT_TIME', 900); // 15 minutes

// File uploads
define('MAX_PLUGIN_FILE_SIZE', 100 * 1024 * 1024); // 100MB
define('MAX_IMAGE_FILE_SIZE', 5 * 1024 * 1024); // 5MB
define('ALLOWED_PLUGIN_EXTENSIONS', ['zip', 'rar', '7z', 'dll', 'vdjplugin', 'vdjsample', 'vdjpad']);
define('ALLOWED_IMAGE_EXTENSIONS', ['jpg', 'jpeg', 'png', 'gif', 'webp', 'svg']);

// Default license settings
define('DEFAULT_MAX_ACTIVATIONS', 3);

// Pagination
define('ITEMS_PER_PAGE', 12);
define('ADMIN_ITEMS_PER_PAGE', 20);
