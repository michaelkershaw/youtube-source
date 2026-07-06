-- Check for your specific duplicate licenses
USE `djmickyk_mrk-vdj-plugins`;

-- Find all licenses for Telly Media Reborn
SELECT 
    l.id,
    l.license_key,
    l.user_id,
    u.email,
    l.plugin_id,
    p.name as plugin_name,
    l.license_type,
    l.status,
    l.max_activations,
    l.devices,
    l.max_activations_updated,
    l.created_at
FROM licenses l
JOIN users u ON l.user_id = u.id
JOIN plugins p ON l.plugin_id = p.id
WHERE p.name = 'Telly Media Reborn'
ORDER BY l.user_id, l.created_at DESC;

-- Count duplicates by user
SELECT 
    u.email,
    p.name as plugin_name,
    COUNT(*) as license_count,
    GROUP_CONCAT(l.license_key) as all_keys
FROM licenses l
JOIN users u ON l.user_id = u.id
JOIN plugins p ON l.plugin_id = p.id
WHERE p.name = 'Telly Media Reborn'
GROUP BY u.id, p.id
HAVING COUNT(*) > 1;
