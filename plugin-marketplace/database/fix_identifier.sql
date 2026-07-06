-- Fix identifier column for existing plugins
-- Step 1: Add column as nullable first
ALTER TABLE plugins ADD COLUMN identifier VARCHAR(255) NULL AFTER slug;

-- Step 2: Update existing plugins to use their slug as identifier
UPDATE plugins SET identifier = slug WHERE identifier IS NULL;

-- Step 3: Now make it NOT NULL and UNIQUE
ALTER TABLE plugins MODIFY COLUMN identifier VARCHAR(255) NOT NULL;
ALTER TABLE plugins ADD UNIQUE KEY unique_identifier (identifier);
