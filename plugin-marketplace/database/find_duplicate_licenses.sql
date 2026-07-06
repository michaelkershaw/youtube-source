-- Find and analyze duplicate licenses
USE `djmickyk_mrk-vdj-plugins`;

-- Find duplicate licenses by user_id + plugin_id
SELECT 
    l.user_id,
    u.email as user_email,
    u.name as user_name,
    l.plugin_id,
    p.name as plugin_name,
    COUNT(*) as duplicate_count,
    GROUP_CONCAT(l.id ORDER BY l.created_at DESC) as license_ids,
    GROUP_CONCAT(l.license_key ORDER BY l.created_at DESC) as license_keys,
    GROUP_CONCAT(l.status ORDER BY l.created_at DESC) as statuses,
    GROUP_CONCAT(DATE(l.created_at) ORDER BY l.created_at DESC) as created_dates,
    MAX(l.created_at) as latest_created,
    MIN(l.created_at) as oldest_created
FROM licenses l
JOIN users u ON l.user_id = u.id
JOIN plugins p ON l.plugin_id = p.id
GROUP BY l.user_id, l.plugin_id
HAVING COUNT(*) > 1
ORDER BY duplicate_count DESC, user_email, plugin_name;

-- Show all duplicate license details
SELECT 
    l.id,
    l.license_key,
    l.status,
    l.license_type,
    l.created_at,
    l.expires_at,
    l.max_activations,
    l.devices,
    l.max_activations_updated,
    u.email as user_email,
    u.name as user_name,
    p.name as plugin_name
FROM licenses l
JOIN users u ON l.user_id = u.id
JOIN plugins p ON l.plugin_id = p.id
WHERE (l.user_id, l.plugin_id) IN (
    SELECT user_id, plugin_id
    FROM licenses
    GROUP BY user_id, plugin_id
    HAVING COUNT(*) > 1
)
ORDER BY l.user_id, l.plugin_id, l.created_at DESC;

-- Count total duplicates
SELECT 
    COUNT(*) as total_duplicate_groups,
    SUM(duplicate_count - 1) as total_extra_duplicates
FROM (
    SELECT COUNT(*) as duplicate_count
    FROM licenses
    GROUP BY user_id, plugin_id
    HAVING COUNT(*) > 1
) as duplicates;
