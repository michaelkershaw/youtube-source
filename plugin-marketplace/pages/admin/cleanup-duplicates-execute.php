<?php
require_admin();

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    redirect('/admin/licenses');
}

// Create backup first
Database::query("
    CREATE TABLE IF NOT EXISTS licenses_duplicate_backup AS
    SELECT l.*
    FROM licenses l
    WHERE (l.user_id, l.plugin_id) IN (
        SELECT user_id, plugin_id
        FROM licenses
        GROUP BY user_id, plugin_id
        HAVING COUNT(*) > 1
    )
");

// Delete duplicates, keeping the most recent active license
$deleted1 = Database::query("
    DELETE l1 FROM licenses l1
    INNER JOIN licenses l2
    WHERE l1.user_id = l2.user_id
      AND l1.plugin_id = l2.plugin_id
      AND l1.id < l2.id
      AND NOT (l1.status = 'active' AND l2.status != 'active')
");

// Delete remaining duplicates with same status, keeping most recent
$deleted2 = Database::query("
    DELETE l1 FROM licenses l1
    INNER JOIN licenses l2
    WHERE l1.user_id = l2.user_id
      AND l1.plugin_id = l2.plugin_id
      AND l1.id < l2.id
      AND l1.status = l2.status
");

$totalDeleted = $deleted1 + $deleted2;

set_flash('success', "Cleaned up {$totalDeleted} duplicate licenses. Backup created in licenses_duplicate_backup table.");
redirect('/admin/licenses');
?>
