<?php
require_admin();
if ($method !== 'POST') redirect('/admin/orders');
require_csrf();

$id = (int)($_GET['id'] ?? 0);
$order = Database::fetch("SELECT * FROM orders WHERE id = ? AND status = 'completed'", [$id]);

if (!$order) {
    set_flash('error', 'Order not found or already refunded.');
    redirect('/admin/orders');
}

// Process refund (mock)
$refundId = 'REFUND-' . strtoupper(bin2hex(random_bytes(8)));

Database::update('orders', [
    'status' => 'refunded',
    'refunded_at' => date('Y-m-d H:i:s'),
    'notes' => ($order['notes'] ? $order['notes'] . "\n" : '') . 'Refunded: ' . $refundId,
    'updated_at' => date('Y-m-d H:i:s'),
], 'id = ?', [$id]);

// Revoke the associated license
$license = Database::fetch("SELECT * FROM licenses WHERE order_id = ?", [$id]);
if ($license) {
    Database::update('licenses', [
        'status' => 'revoked',
        'updated_at' => date('Y-m-d H:i:s'),
    ], 'id = ?', [$license['id']]);
}

// Decrement sales count
Database::query("UPDATE plugins SET sales_count = GREATEST(sales_count - 1, 0) WHERE id = ?", [$order['plugin_id']]);

set_flash('success', 'Order refunded successfully. License has been revoked.');
redirect('/admin/orders/' . $id);
