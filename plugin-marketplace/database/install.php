<?php
/**
 * Plugin Marketplace - Install & Seed Script
 * 
 * Run this once to set up the database and create demo data.
 * Usage: php install.php  OR  visit /install.php in browser
 * 
 * DELETE THIS FILE AFTER INSTALLATION!
 */

$isCli = php_sapi_name() === 'cli';

if (!$isCli) {
    echo '<!DOCTYPE html><html><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Install</title>';
    echo '<script src="https://cdn.tailwindcss.com"></script>';
    echo '<link href="https://fonts.bunny.net/css?family=inter:400,600,700&display=swap" rel="stylesheet"/>';
    echo '</head><body class="bg-gray-950 text-gray-100 font-sans p-8"><div class="max-w-2xl mx-auto">';
    echo '<h1 class="text-3xl font-bold text-white mb-6">Plugin Marketplace - Installer</h1>';
    echo '<div class="bg-gray-900 border border-gray-800 rounded-2xl p-6 space-y-2 text-sm font-mono">';
}

function out($msg, $type = 'info') {
    global $isCli;
    if ($isCli) {
        echo ($type === 'error' ? '[ERROR] ' : ($type === 'success' ? '[OK] ' : '[INFO] ')) . $msg . "\n";
    } else {
        $color = match($type) {
            'error' => 'text-red-400',
            'success' => 'text-emerald-400',
            default => 'text-gray-400',
        };
        echo "<div class=\"{$color}\">" . htmlspecialchars($msg) . "</div>";
    }
}

// Load configuration
require_once __DIR__ . '/config.php';

// --- Step 1: Connect to MySQL ---
try {
    $pdo = new PDO(
        'mysql:host=' . DB_HOST . ';port=' . DB_PORT . ';charset=' . DB_CHARSET,
        DB_USER,
        DB_PASS,
        [PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION]
    );
    out('Connected to MySQL.', 'success');
} catch (PDOException $e) {
    out('Cannot connect to MySQL: ' . $e->getMessage(), 'error');
    out('Make sure MySQL is running and update credentials in config.php', 'error');
    exit(1);
}

// --- Step 2: Create Database ---
$pdo->exec("CREATE DATABASE IF NOT EXISTS `" . DB_NAME . "` CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci");
$pdo->exec("USE `" . DB_NAME . "`");
out('Database "' . DB_NAME . '" ready.', 'success');

// --- Step 3: Run Schema ---
$schemaFile = __DIR__ . '/database/schema.sql';
if (!file_exists($schemaFile)) {
    out('schema.sql not found!', 'error');
    exit(1);
}

$schema = file_get_contents($schemaFile);
// Remove CREATE DATABASE and USE lines (already handled)
$schema = preg_replace('/^(CREATE DATABASE|USE ).*/m', '', $schema);
// Split by semicolons
$statements = array_filter(array_map('trim', explode(';', $schema)));
$errors = 0;
foreach ($statements as $stmt) {
    if (empty($stmt) || $stmt === '--') continue;
    try {
        $pdo->exec($stmt);
    } catch (PDOException $e) {
        // Ignore duplicate index/table errors
        if (strpos($e->getMessage(), 'Duplicate') === false && strpos($e->getMessage(), 'already exists') === false) {
            out('SQL Error: ' . $e->getMessage(), 'error');
            $errors++;
        }
    }
}
out('Schema tables created. (' . ($errors ? "$errors warnings" : 'clean') . ')', $errors ? 'info' : 'success');

// --- Step 4: Seed Demo Data ---
out('Seeding demo data...', 'info');

// Admin user
$adminExists = $pdo->query("SELECT COUNT(*) FROM users WHERE email = 'admin@example.com'")->fetchColumn();
if (!$adminExists) {
    $pdo->prepare("INSERT INTO users (name, email, password, role, is_active, created_at, updated_at) VALUES (?, ?, ?, 'admin', 1, NOW(), NOW())")
        ->execute(['Admin', 'admin@example.com', password_hash('admin123', PASSWORD_DEFAULT)]);
    out('Admin user created: admin@example.com / admin123', 'success');
} else {
    out('Admin user already exists.', 'info');
}

// Demo user
$demoExists = $pdo->query("SELECT COUNT(*) FROM users WHERE email = 'user@example.com'")->fetchColumn();
if (!$demoExists) {
    $pdo->prepare("INSERT INTO users (name, email, password, role, is_active, created_at, updated_at) VALUES (?, ?, ?, 'user', 1, NOW(), NOW())")
        ->execute(['Demo User', 'user@example.com', password_hash('user123', PASSWORD_DEFAULT)]);
    out('Demo user created: user@example.com / user123', 'success');
} else {
    out('Demo user already exists.', 'info');
}

// Demo Plugins
$pluginCount = $pdo->query("SELECT COUNT(*) FROM plugins")->fetchColumn();
if ($pluginCount == 0) {
    $demoPlugins = [
        ['name' => 'AutoMix Pro', 'slug' => 'automix-pro', 'description' => 'Advanced automatic mixing with AI-powered transitions, beat matching, and harmonic mixing capabilities.', 'category' => 'Effects', 'is_featured' => 1],
        ['name' => 'VDJ Skin Designer', 'slug' => 'vdj-skin-designer', 'description' => 'Create stunning custom skins for your DJ software with drag-and-drop interface and live preview.', 'category' => 'Skins', 'is_featured' => 1],
        ['name' => 'BeatSync Visualizer', 'slug' => 'beatsync-visualizer', 'description' => 'Real-time audio visualization synchronized to your beats with customizable effects and overlays.', 'category' => 'Visuals', 'is_featured' => 1],
        ['name' => 'Sample Pack Manager', 'slug' => 'sample-pack-manager', 'description' => 'Organize, tag, and quickly access your sample packs with intelligent search and categorization.', 'category' => 'Tools', 'is_featured' => 0],
        ['name' => 'Key Detector Plus', 'slug' => 'key-detector-plus', 'description' => 'Accurate musical key detection with Camelot wheel display and harmonic mixing suggestions.', 'category' => 'Effects', 'is_featured' => 0],
        ['name' => 'Stream Connector', 'slug' => 'stream-connector', 'description' => 'Stream your DJ sets directly to Twitch, YouTube, and Facebook with overlay and chat integration.', 'category' => 'Tools', 'is_featured' => 1],
    ];

    foreach ($demoPlugins as $dp) {
        $pdo->prepare(
            "INSERT INTO plugins (name, slug, description, category, is_active, is_featured, downloads_count, sales_count, created_at, updated_at) 
             VALUES (?, ?, ?, ?, 1, ?, ?, ?, NOW(), NOW())"
        )->execute([
            $dp['name'], $dp['slug'], $dp['description'], $dp['category'], $dp['is_featured'],
            rand(50, 5000), rand(10, 500)
        ]);
        $pluginId = $pdo->lastInsertId();

        // Add a version
        $pdo->prepare(
            "INSERT INTO plugin_versions (plugin_id, version, changelog, file_path, file_name, file_size, is_latest, is_active, released_at, created_at) 
             VALUES (?, '1.0.0', 'Initial release with core functionality.', '', '', 0, 1, 1, NOW(), NOW())"
        )->execute([$pluginId]);

        // Add pricing
        $basePrice = rand(5, 25);
        $pdo->prepare("INSERT INTO plugin_pricings (plugin_id, license_type, price, is_active, created_at, updated_at) VALUES (?, 'monthly', ?, 1, NOW(), NOW())")->execute([$pluginId, $basePrice]);
        $pdo->prepare("INSERT INTO plugin_pricings (plugin_id, license_type, price, is_active, created_at, updated_at) VALUES (?, 'yearly', ?, 1, NOW(), NOW())")->execute([$pluginId, $basePrice * 10]);
        $pdo->prepare("INSERT INTO plugin_pricings (plugin_id, license_type, price, is_active, created_at, updated_at) VALUES (?, 'lifetime', ?, 1, NOW(), NOW())")->execute([$pluginId, $basePrice * 25]);
    }
    out(count($demoPlugins) . ' demo plugins created with versions and pricing.', 'success');
} else {
    out('Plugins already exist, skipping seed.', 'info');
}

// Create upload directories
foreach ([
    __DIR__ . '/uploads',
    __DIR__ . '/uploads/plugins',
    __DIR__ . '/uploads/images',
] as $dir) {
    if (!is_dir($dir)) {
        mkdir($dir, 0755, true);
        out("Created directory: $dir", 'success');
    }
}

// --- Done ---
out('', 'info');
out('========================================', 'success');
out('Installation complete!', 'success');
out('========================================', 'success');
out('', 'info');
out('Admin Login: admin@example.com / admin123', 'info');
out('Demo Login:  user@example.com / user123', 'info');
out('', 'info');
out('IMPORTANT: Delete install.php after setup!', 'error');

if (!$isCli) {
    echo '</div>';
    echo '<div class="mt-6 flex gap-4">';
    echo '<a href="/" class="px-6 py-3 bg-indigo-600 hover:bg-indigo-500 text-white font-semibold rounded-xl transition">Go to Site</a>';
    echo '<a href="/login" class="px-6 py-3 bg-gray-800 hover:bg-gray-700 text-white font-medium rounded-xl transition border border-gray-700">Login</a>';
    echo '</div>';
    echo '</div></body></html>';
}
