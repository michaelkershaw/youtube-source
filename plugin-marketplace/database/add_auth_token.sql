-- Add auth_token column to licenses table for account-based authentication
ALTER TABLE licenses ADD COLUMN auth_token VARCHAR(128) NULL AFTER license_key;
ALTER TABLE licenses ADD COLUMN last_login_at DATETIME NULL AFTER auth_token;

-- Add index for faster auth token lookups
ALTER TABLE licenses ADD INDEX idx_auth_token (auth_token);
