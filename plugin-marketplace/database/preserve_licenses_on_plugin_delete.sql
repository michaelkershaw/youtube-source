-- Migration: Preserve licenses and downloads when plugin is deleted
-- This changes foreign key constraints from CASCADE to SET NULL
-- Run this on your production database

USE `djmickyk_mrk-vdj-plugins`;

-- Step 1: Drop existing foreign keys that CASCADE delete
ALTER TABLE `licenses` DROP FOREIGN KEY `licenses_ibfk_2`;
ALTER TABLE `downloads` DROP FOREIGN KEY `downloads_ibfk_2`;
ALTER TABLE `orders` DROP FOREIGN KEY `orders_ibfk_2`;

-- Step 2: Make plugin_id nullable in licenses and downloads (so they can be preserved)
ALTER TABLE `licenses` MODIFY `plugin_id` INT UNSIGNED NULL;
ALTER TABLE `downloads` MODIFY `plugin_id` INT UNSIGNED NULL;
ALTER TABLE `orders` MODIFY `plugin_id` INT UNSIGNED NULL;

-- Step 3: Re-add foreign keys with SET NULL instead of CASCADE
ALTER TABLE `licenses` 
    ADD CONSTRAINT `licenses_ibfk_2` 
    FOREIGN KEY (`plugin_id`) REFERENCES `plugins`(`id`) 
    ON DELETE SET NULL;

ALTER TABLE `downloads` 
    ADD CONSTRAINT `downloads_ibfk_2` 
    FOREIGN KEY (`plugin_id`) REFERENCES `plugins`(`id`) 
    ON DELETE SET NULL;

ALTER TABLE `orders` 
    ADD CONSTRAINT `orders_ibfk_2` 
    FOREIGN KEY (`plugin_id`) REFERENCES `plugins`(`id`) 
    ON DELETE SET NULL;

-- Verification: Check the new constraints
SELECT 
    TABLE_NAME,
    COLUMN_NAME,
    CONSTRAINT_NAME,
    REFERENCED_TABLE_NAME,
    REFERENCED_COLUMN_NAME,
    DELETE_RULE
FROM 
    INFORMATION_SCHEMA.KEY_COLUMN_USAGE
WHERE 
    TABLE_SCHEMA = 'djmickyk_mrk-vdj-plugins'
    AND REFERENCED_TABLE_NAME = 'plugins'
    AND TABLE_NAME IN ('licenses', 'downloads', 'orders');
