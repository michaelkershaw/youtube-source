-- Check current device activations
SELECT 
    l.id,
    l.license_key,
    l.devices as current_devices_column,
    COUNT(da.id) as actual_device_count
FROM licenses l
LEFT JOIN device_activations da ON da.license_id = l.id AND da.is_active = 1
GROUP BY l.id;

-- Sync devices column with actual device_activations count
UPDATE licenses l
SET l.devices = (
    SELECT COUNT(*) 
    FROM device_activations da 
    WHERE da.license_id = l.id AND da.is_active = 1
);

-- Verify the update
SELECT 
    l.id,
    l.license_key,
    l.devices,
    (SELECT COUNT(*) FROM device_activations da WHERE da.license_id = l.id AND da.is_active = 1) as device_count
FROM licenses l;
