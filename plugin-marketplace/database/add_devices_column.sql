-- Add devices column to licenses table to track device activation count
ALTER TABLE licenses ADD COLUMN devices INT NOT NULL DEFAULT 0 AFTER max_activations;

-- Update existing licenses to set devices = 0 if NULL
UPDATE licenses SET devices = 0 WHERE devices IS NULL;
