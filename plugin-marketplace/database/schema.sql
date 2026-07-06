-- Plugin Marketplace Database Schema
-- MySQL 5.7+ / MariaDB 10.3+

CREATE DATABASE IF NOT EXISTS `djmickyk_mrk-vdj-plugins` CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE `djmickyk_mrk-vdj-plugins`;

-- Users
CREATE TABLE IF NOT EXISTS users (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    email VARCHAR(255) NOT NULL UNIQUE,
    password VARCHAR(255) NOT NULL,
    role ENUM('user', 'admin') NOT NULL DEFAULT 'user',
    is_active TINYINT(1) NOT NULL DEFAULT 1,
    reset_token VARCHAR(255) NULL,
    reset_token_expires DATETIME NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB;

-- Plugins
CREATE TABLE IF NOT EXISTS plugins (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    slug VARCHAR(255) NOT NULL UNIQUE,
    description TEXT NOT NULL,
    full_description LONGTEXT NULL,
    category VARCHAR(100) NULL,
    is_active TINYINT(1) NOT NULL DEFAULT 1,
    is_featured TINYINT(1) NOT NULL DEFAULT 0,
    downloads_count INT UNSIGNED NOT NULL DEFAULT 0,
    sales_count INT UNSIGNED NOT NULL DEFAULT 0,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB;

-- Plugin Versions
CREATE TABLE IF NOT EXISTS plugin_versions (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    plugin_id INT UNSIGNED NOT NULL,
    version VARCHAR(20) NOT NULL,
    changelog TEXT NULL,
    file_path VARCHAR(500) NOT NULL DEFAULT '',
    file_name VARCHAR(255) NOT NULL DEFAULT '',
    file_size INT UNSIGNED NULL DEFAULT 0,
    file_hash VARCHAR(64) NULL,
    is_latest TINYINT(1) NOT NULL DEFAULT 0,
    is_active TINYINT(1) NOT NULL DEFAULT 1,
    released_at DATETIME NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uq_plugin_version (plugin_id, version),
    FOREIGN KEY (plugin_id) REFERENCES plugins(id) ON DELETE CASCADE
) ENGINE=InnoDB;

-- Plugin Images
CREATE TABLE IF NOT EXISTS plugin_images (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    plugin_id INT UNSIGNED NOT NULL,
    image_path VARCHAR(500) NOT NULL,
    image_name VARCHAR(255) NOT NULL,
    caption VARCHAR(255) NULL,
    sort_order INT NOT NULL DEFAULT 0,
    is_featured TINYINT(1) NOT NULL DEFAULT 0,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (plugin_id) REFERENCES plugins(id) ON DELETE CASCADE
) ENGINE=InnoDB;

-- Plugin Pricing
CREATE TABLE IF NOT EXISTS plugin_pricings (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    plugin_id INT UNSIGNED NOT NULL,
    license_type ENUM('monthly', 'yearly', 'lifetime', 'custom_days') NOT NULL,
    custom_days INT NULL,
    price DECIMAL(10,2) NOT NULL,
    is_active TINYINT(1) NOT NULL DEFAULT 1,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    UNIQUE KEY uq_plugin_pricing (plugin_id, license_type, custom_days),
    FOREIGN KEY (plugin_id) REFERENCES plugins(id) ON DELETE CASCADE
) ENGINE=InnoDB;

-- Orders
CREATE TABLE IF NOT EXISTS orders (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id INT UNSIGNED NOT NULL,
    plugin_id INT UNSIGNED NOT NULL,
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
    FOREIGN KEY (plugin_id) REFERENCES plugins(id) ON DELETE CASCADE,
    FOREIGN KEY (plugin_pricing_id) REFERENCES plugin_pricings(id) ON DELETE CASCADE
) ENGINE=InnoDB;

-- Licenses
CREATE TABLE IF NOT EXISTS licenses (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id INT UNSIGNED NOT NULL,
    plugin_id INT UNSIGNED NOT NULL,
    order_id INT UNSIGNED NULL,
    license_key VARCHAR(255) NOT NULL UNIQUE,
    license_type ENUM('monthly', 'yearly', 'lifetime', 'custom_days') NOT NULL,
    custom_days INT NULL,
    status ENUM('active', 'expired', 'revoked') NOT NULL DEFAULT 'active',
    starts_at DATETIME NOT NULL,
    expires_at DATETIME NULL,
    max_activations INT NOT NULL DEFAULT 3,
    current_activations INT NOT NULL DEFAULT 0,
    last_activated_at DATETIME NULL,
    notes TEXT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (plugin_id) REFERENCES plugins(id) ON DELETE CASCADE,
    FOREIGN KEY (order_id) REFERENCES orders(id) ON DELETE SET NULL
) ENGINE=InnoDB;

-- License Formats (per-plugin key format)
CREATE TABLE IF NOT EXISTS license_formats (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    plugin_id INT UNSIGNED NOT NULL UNIQUE,
    prefix VARCHAR(20) NULL,
    format_pattern VARCHAR(100) NOT NULL DEFAULT 'XXXX-XXXX-XXXX-XXXX',
    validation_rules TEXT NULL,
    segment_length INT NOT NULL DEFAULT 4,
    `separator` VARCHAR(5) NOT NULL DEFAULT '-',
    is_active TINYINT(1) NOT NULL DEFAULT 1,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (plugin_id) REFERENCES plugins(id) ON DELETE CASCADE
) ENGINE=InnoDB;

-- Downloads
CREATE TABLE IF NOT EXISTS downloads (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id INT UNSIGNED NOT NULL,
    plugin_id INT UNSIGNED NOT NULL,
    plugin_version_id INT UNSIGNED NOT NULL,
    license_id INT UNSIGNED NOT NULL,
    download_token VARCHAR(255) NOT NULL UNIQUE,
    ip_address VARCHAR(45) NULL,
    user_agent VARCHAR(500) NULL,
    downloaded_at DATETIME NOT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (plugin_id) REFERENCES plugins(id) ON DELETE CASCADE,
    FOREIGN KEY (plugin_version_id) REFERENCES plugin_versions(id) ON DELETE CASCADE,
    FOREIGN KEY (license_id) REFERENCES licenses(id) ON DELETE CASCADE
) ENGINE=InnoDB;

-- Device Activations
CREATE TABLE IF NOT EXISTS device_activations (
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

-- Indexes for performance
CREATE INDEX idx_plugins_active ON plugins(is_active);
CREATE INDEX idx_plugins_featured ON plugins(is_featured);
CREATE INDEX idx_plugins_slug ON plugins(slug);
CREATE INDEX idx_licenses_user ON licenses(user_id);
CREATE INDEX idx_licenses_plugin ON licenses(plugin_id);
CREATE INDEX idx_licenses_key ON licenses(license_key);
CREATE INDEX idx_licenses_status ON licenses(status);
CREATE INDEX idx_orders_user ON orders(user_id);
CREATE INDEX idx_orders_status ON orders(status);
CREATE INDEX idx_downloads_user ON downloads(user_id);
CREATE INDEX idx_device_activations_license ON device_activations(license_id);
