-- Fix device slots upgrade system - add missing columns
USE `djmickyk_mrk-vdj-plugins`;

-- Add missing columns to licenses table
ALTER TABLE `licenses` 
ADD COLUMN IF NOT EXISTS `max_activations_updated` INT NULL COMMENT 'Updated max activations after upgrades',
ADD COLUMN IF NOT EXISTS `upgrade_history` JSON NULL COMMENT 'History of device slot upgrades';

-- Fix device_slots_upgrades table (add missing user_id column for security)
ALTER TABLE `device_slots_upgrades` 
ADD COLUMN IF NOT EXISTS `user_id` INT UNSIGNED NULL AFTER `license_id`,
ADD FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE;

-- Update existing records to set user_id
UPDATE device_slots_upgrades dsu 
JOIN licenses l ON dsu.license_id = l.id 
SET dsu.user_id = l.user_id 
WHERE dsu.user_id IS NULL;

-- Make user_id NOT NULL after updating
ALTER TABLE `device_slots_upgrades` 
MODIFY COLUMN `user_id` INT UNSIGNED NOT NULL;

-- Verification
SELECT 'Device slots system fixed!' as status;
SHOW COLUMNS FROM licenses WHERE Field IN ('max_activations_updated', 'upgrade_history');
SHOW COLUMNS FROM device_slots_upgrades WHERE Field = 'user_id';
