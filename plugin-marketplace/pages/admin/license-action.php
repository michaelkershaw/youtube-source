<?php
require_admin();
if ($method !== 'POST') redirect('/admin/licenses');
require_csrf();

$id = (int)($_GET['id'] ?? 0);
$license = Database::fetch("SELECT * FROM licenses WHERE id = ?", [$id]);
if (!$license) { set_flash('error', 'License not found.'); redirect('/admin/licenses'); }

$action = $_POST['action'] ?? '';

switch ($action) {
    case 'revoke':
        Database::update('licenses', [
            'status' => 'revoked',
            'updated_at' => date('Y-m-d H:i:s'),
        ], 'id = ?', [$id]);
        set_flash('success', 'License revoked.');
        break;

    case 'activate':
        $expiresAt = $license['expires_at'];
        // If was expired, extend from now
        if ($expiresAt && strtotime($expiresAt) < time()) {
            $expiresAt = LicenseKeyGenerator::calculateExpiration(
                $license['license_type'],
                $license['custom_days']
            );
        }
        Database::update('licenses', [
            'status' => 'active',
            'expires_at' => $expiresAt,
            'updated_at' => date('Y-m-d H:i:s'),
        ], 'id = ?', [$id]);
        set_flash('success', 'License reactivated.');
        break;

    case 'extend':
        $days = max(1, (int)($_POST['days'] ?? 30));
        $currentExpiry = $license['expires_at'] ?: date('Y-m-d H:i:s');
        $base = max(strtotime($currentExpiry), time());
        $newExpiry = date('Y-m-d H:i:s', strtotime("+{$days} days", $base));
        Database::update('licenses', [
            'expires_at' => $newExpiry,
            'status' => 'active',
            'updated_at' => date('Y-m-d H:i:s'),
        ], 'id = ?', [$id]);
        set_flash('success', 'License extended by ' . $days . ' days. New expiry: ' . format_date($newExpiry));
        break;

    case 'reset_activations':
        Database::update('device_activations', [
            'is_active' => 0,
            'deactivated_at' => date('Y-m-d H:i:s'),
        ], 'license_id = ? AND is_active = 1', [$id]);
        Database::update('licenses', [
            'current_activations' => 0,
            'updated_at' => date('Y-m-d H:i:s'),
        ], 'id = ?', [$id]);
        set_flash('success', 'All device activations reset.');
        break;

    case 'set_max_activations':
        $max = max(1, (int)($_POST['max_activations'] ?? DEFAULT_MAX_ACTIVATIONS));
        Database::update('licenses', [
            'max_activations' => $max,
            'updated_at' => date('Y-m-d H:i:s'),
        ], 'id = ?', [$id]);
        set_flash('success', 'Max activations set to ' . $max . '.');
        break;

    default:
        set_flash('error', 'Unknown action.');
}

redirect('/admin/licenses/' . $id);
