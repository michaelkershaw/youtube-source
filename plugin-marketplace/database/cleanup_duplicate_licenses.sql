-- Clean up duplicate licenses - KEEP THE MOST RECENT ACTIVE LICENSE
-- DANGER: This will delete duplicate licenses - backup first!

USE `djmickyk_mrk-vdj-plugins`;

-- Create backup of duplicates before deletion
CREATE TABLE licenses_duplicate_backup AS
SELECT l.*
FROM licenses l
WHERE (l.user_id, l.plugin_id) IN (
    SELECT user_id, plugin_id
    FROM licenses
    GROUP BY user_id, plugin_id
    HAVING COUNT(*) > 1
);

-- Delete duplicates, keeping the most recent active license for each user/plugin pair
DELETE l1 FROM licenses l1
INNER JOIN licenses l2
WHERE l1.user_id = l2.user_id
  AND l1.plugin_id = l2.plugin_id
  AND l1.id < l2.id  -- Keep the one with higher ID (more recent)
  AND NOT (l1.status = 'active' AND l2.status != 'active');  -- But keep active ones

-- If there are still duplicates with same status, keep the most recent by created_at
DELETE l1 FROM licenses l1
INNER JOIN licenses l2
WHERE l1.user_id = l2.user_id
  AND l1.plugin_id = l2.plugin_id
  AND l1.id < l2.id
  AND l1.status = l2.status;

-- Show remaining duplicates (if any)
SELECT 
    l.user_id,
    u.email,
    p.name as plugin_name,
    COUNT(*) as remaining_count
FROM licenses l
JOIN users u ON l.user_id = u.id
JOIN plugins p ON l.plugin_id = p.id
GROUP BY l.user_id, l.plugin_id
HAVING COUNT(*) > 1;

-- Show cleanup results
SELECT 
    'Cleanup completed' as status,
    (SELECT COUNT(*) FROM licenses_duplicate_backup) as licenses_backed_up,
    (SELECT COUNT(*) FROM licenses) as remaining_licenses;
