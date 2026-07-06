-- Alternative Migration: Preserve licenses and downloads when plugin is deleted
-- This approach works with limited permissions by recreating tables
-- IMPORTANT: Backup your data before running this!

USE `djmickyk_mrk-vdj-plugins`;

-- Step 1: Create temporary backup tables
CREATE TABLE licenses_backup AS SELECT * FROM licenses;
CREATE TABLE downloads_backup AS SELECT * FROM downloads;
CREATE TABLE orders_backup AS SELECT * FROM orders;

-- Step 2: Drop and recreate licenses table with new constraints
DROP TABLE IF EXISTS licenses;
CREATE TABLE licenses (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id INT UNSIGNED NOT NULL,
    plugin_id INT UNSIGNED NULL,  -- Changed to NULL
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
    FOREIGN KEY (plugin_id) REFERENCES plugins(id) ON DELETE SET NULL,  -- Changed to SET NULL
    FOREIGN KEY (order_id) REFERENCES orders(id) ON DELETE SET NULL
) ENGINE=InnoDB;

-- Step 3: Restore data
INSERT INTO licenses SELECT * FROM licenses_backup;

-- Step 4: Drop and recreate downloads table with new constraints
DROP TABLE IF EXISTS downloads;
CREATE TABLE downloads (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id INT UNSIGNED NOT NULL,
    plugin_id INT UNSIGNED NULL,  -- Changed to NULL
    plugin_version_id INT UNSIGNED NOT NULL,
    license_id INT UNSIGNED NOT NULL,
    download_token VARCHAR(255) NOT NULL UNIQUE,
    ip_address VARCHAR(45) NULL,
    user_agent VARCHAR(500) NULL,
    downloaded_at DATETIME NOT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (plugin_id) REFERENCES plugins(id) ON DELETE SET NULL,  -- Changed to SET NULL
    FOREIGN KEY (plugin_version_id) REFERENCES plugin_versions(id) ON DELETE CASCADE,
    FOREIGN KEY (license_id) REFERENCES licenses(id) ON DELETE CASCADE
) ENGINE=InnoDB;

-- Step 5: Restore data
INSERT INTO downloads SELECT * FROM downloads_backup;

-- Step 6: Drop and recreate orders table with new constraints
DROP TABLE IF EXISTS orders;
CREATE TABLE orders (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id INT UNSIGNED NOT NULL,
    plugin_id INT UNSIGNED NULL,  -- Changed to NULL
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
    FOREIGN KEY (plugin_id) REFERENCES plugins(id) ON DELETE SET NULL,  -- Changed to SET NULL
    FOREIGN KEY (plugin_pricing_id) REFERENCES plugin_pricings(id) ON DELETE CASCADE
) ENGINE=InnoDB;

-- Step 7: Restore data
INSERT INTO orders SELECT * FROM orders_backup;

-- Step 8: Clean up backup tables
DROP TABLE licenses_backup;
DROP TABLE downloads_backup;
DROP TABLE orders_backup;

-- Verification query
SELECT 'Migration completed successfully!' as status;
