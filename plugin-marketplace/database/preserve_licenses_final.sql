-- Final Migration: Preserve licenses and downloads when plugin is deleted
-- This handles all foreign key dependencies correctly
-- IMPORTANT: Backup your data before running this!

USE `djmickyk_mrk-vdj-plugins`;

-- Step 1: Create backup tables
CREATE TABLE licenses_backup AS SELECT * FROM licenses;
CREATE TABLE downloads_backup AS SELECT * FROM downloads;
CREATE TABLE orders_backup AS SELECT * FROM orders;
CREATE TABLE device_activations_backup AS SELECT * FROM device_activations;

-- Step 2: Drop tables in correct order (child tables first)
DROP TABLE IF EXISTS device_activations;
DROP TABLE IF EXISTS downloads;
DROP TABLE IF EXISTS licenses;
DROP TABLE IF EXISTS orders;

-- Step 3: Recreate orders table with SET NULL
CREATE TABLE orders (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id INT UNSIGNED NOT NULL,
    plugin_id INT UNSIGNED NULL,
    plugin_pricing_id INT UNSIGNED NOT NULL,
    order_number VARCHAR(50) NOT NULL UNIQUE,
    license_type ENUM('monthly', 'yearly', 'lifetime', 'custom_days') NOT NULL,
    custom_days INT NULL,
    price DECIMAL(10,2) NOT NULL,
    status ENUM('pending', 'completed', 'failed', 'refunded') NOT NULL DEFAULT 'pending',
    payment_method VARCHAR(50) NULL,
    transaction_id VARCHAR(255) NULL,
    paid_at DATETIME NULL,
    refunded_at DATETIME NULL,
    notes TEXT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (plugin_id) REFERENCES plugins(id) ON DELETE SET NULL,
    FOREIGN KEY (plugin_pricing_id) REFERENCES plugin_pricings(id) ON DELETE CASCADE
) ENGINE=InnoDB;

-- Step 4: Recreate licenses table with SET NULL
CREATE TABLE licenses (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id INT UNSIGNED NOT NULL,
    plugin_id INT UNSIGNED NULL,
    order_id INT UNSIGNED NULL,
    license_key VARCHAR(255) NOT NULL UNIQUE,
    license_type ENUM('monthly', 'yearly', 'lifetime', 'custom_days') NOT NULL,
    custom_days INT NULL,
    status ENUM('active', 'expired', 'revoked') NOT NULL DEFAULT 'active',
    starts_at DATETIME NOT NULL,
    expires_at DATETIME NULL,
    max_activations INT NOT NULL DEFAULT 3,
    current_activations INT NOT NULL DEFAULT 0,
    devices INT NOT NULL DEFAULT 0,
    auth_token VARCHAR(255) NULL,
    last_login_at DATETIME NULL,
    last_activated_at DATETIME NULL,
    notes TEXT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (plugin_id) REFERENCES plugins(id) ON DELETE SET NULL,
    FOREIGN KEY (order_id) REFERENCES orders(id) ON DELETE SET NULL
) ENGINE=InnoDB;

-- Step 5: Recreate downloads table with SET NULL
CREATE TABLE downloads (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id INT UNSIGNED NOT NULL,
    plugin_id INT UNSIGNED NULL,
    plugin_version_id INT UNSIGNED NOT NULL,
    license_id INT UNSIGNED NOT NULL,
    download_token VARCHAR(255) NOT NULL UNIQUE,
    ip_address VARCHAR(45) NULL,
    user_agent VARCHAR(500) NULL,
    downloaded_at DATETIME NOT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (plugin_id) REFERENCES plugins(id) ON DELETE SET NULL,
    FOREIGN KEY (plugin_version_id) REFERENCES plugin_versions(id) ON DELETE CASCADE,
    FOREIGN KEY (license_id) REFERENCES licenses(id) ON DELETE CASCADE
) ENGINE=InnoDB;

-- Step 6: Recreate device_activations table
CREATE TABLE device_activations (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    license_id INT UNSIGNED NOT NULL,
    device_id VARCHAR(255) NOT NULL,
    device_name VARCHAR(255) NULL,
    hardware_hash VARCHAR(255) NULL,
    ip_address VARCHAR(45) NULL,
    is_active TINYINT(1) NOT NULL DEFAULT 1,
    activated_at DATETIME NOT NULL,
    last_seen_at DATETIME NULL,
    deactivated_at DATETIME NULL,
    notes TEXT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uq_license_device (license_id, device_id),
    FOREIGN KEY (license_id) REFERENCES licenses(id) ON DELETE CASCADE
) ENGINE=InnoDB;

-- Step 7: Restore data in correct order (parent tables first)
INSERT INTO orders SELECT * FROM orders_backup;
INSERT INTO licenses SELECT * FROM licenses_backup;
INSERT INTO downloads SELECT * FROM downloads_backup;
INSERT INTO device_activations SELECT * FROM device_activations_backup;

-- Step 8: Clean up backup tables
DROP TABLE orders_backup;
DROP TABLE licenses_backup;
DROP TABLE downloads_backup;
DROP TABLE device_activations_backup;

-- Verification
SELECT 'Migration completed successfully!' as status;
SELECT COUNT(*) as licenses_count FROM licenses;
SELECT COUNT(*) as downloads_count FROM downloads;
SELECT COUNT(*) as orders_count FROM orders;
SELECT COUNT(*) as devices_count FROM device_activations;
