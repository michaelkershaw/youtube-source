-- Add device slot upgrade functionality
-- This allows lifetime license holders to purchase additional device slots

USE `djmickyk_mrk-vdj-plugins`;

-- Step 1: Add device_slots_upgrade table
CREATE TABLE IF NOT EXISTS device_slots_upgrades (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    license_id INT UNSIGNED NOT NULL,
    additional_slots INT NOT NULL,
    price DECIMAL(10,2) NOT NULL,
    status ENUM('pending', 'completed', 'failed', 'refunded') NOT NULL DEFAULT 'pending',
    order_number VARCHAR(50) NOT NULL UNIQUE,
    payment_method VARCHAR(50) NULL,
    transaction_id VARCHAR(255) NULL,
    paid_at DATETIME NULL,
    notes TEXT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (license_id) REFERENCES licenses(id) ON DELETE CASCADE,
    INDEX idx_license_upgrade (license_id),
    INDEX idx_order_number (order_number),
    INDEX idx_status (status)
) ENGINE=InnoDB;

-- Step 2: Add max_activations_updated column to licenses to track upgrades
ALTER TABLE `licenses` ADD COLUMN IF NOT EXISTS `max_activations_updated` INT NULL COMMENT 'Updated max activations after upgrades';
ALTER TABLE `licenses` ADD COLUMN IF NOT EXISTS `upgrade_history` JSON NULL COMMENT 'History of device slot upgrades';

-- Step 3: Add device slot pricing configuration table
CREATE TABLE IF NOT EXISTS device_slot_pricing (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    plugin_id INT UNSIGNED NULL,
    slots_count INT NOT NULL COMMENT 'Number of additional slots',
    price DECIMAL(10,2) NOT NULL,
    is_active TINYINT(1) NOT NULL DEFAULT 1,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (plugin_id) REFERENCES plugins(id) ON DELETE CASCADE,
    INDEX idx_plugin_slots (plugin_id),
    INDEX idx_slots_count (slots_count),
    UNIQUE KEY uq_plugin_slots (plugin_id, slots_count)
) ENGINE=InnoDB;

-- Step 4: Insert default device slot pricing (you can adjust these prices)
INSERT INTO device_slot_pricing (plugin_id, slots_count, price) VALUES
(NULL, 1, 5.00),   -- $5 for 1 additional slot (global default)
(NULL, 3, 12.00),  -- $12 for 3 additional slots (save $3)
(NULL, 5, 18.00),  -- $18 for 5 additional slots (save $7)
(NULL, 10, 30.00); -- $30 for 10 additional slots (save $20)

-- Step 5: Create a view to show current license info with upgrade options
CREATE OR REPLACE VIEW license_upgrades_view AS
SELECT 
    l.id as license_id,
    l.license_key,
    l.max_activations,
    l.devices,
    COALESCE(l.max_activations_updated, l.max_activations) as current_max_slots,
    (COALESCE(l.max_activations_updated, l.max_activations) - l.devices) as available_slots,
    l.license_type,
    p.name as plugin_name,
    p.slug as plugin_slug,
    u.name as user_name,
    u.email as user_email,
    CASE 
        WHEN l.license_type = 'lifetime' THEN 1 
        ELSE 0 
    END as can_upgrade,
    l.upgrade_history
FROM licenses l
JOIN plugins p ON l.plugin_id = p.id
JOIN users u ON l.user_id = u.id
WHERE l.status = 'active' AND (l.expires_at IS NULL OR l.expires_at > NOW());

-- Verification
SELECT 'Device slot upgrade system created successfully!' as status;
SELECT COUNT(*) as pricing_options FROM device_slot_pricing WHERE is_active = 1;
