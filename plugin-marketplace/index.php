<?php
/**
 * Plugin Marketplace - Front Controller
 * Pure PHP + MySQL + Tailwind CSS
 */

require_once __DIR__ . '/config.php';
require_once __DIR__ . '/includes/db.php';
require_once __DIR__ . '/includes/auth.php';
require_once __DIR__ . '/includes/helpers.php';
require_once __DIR__ . '/includes/license.php';

start_session();

// Create upload directories if needed
foreach ([UPLOADS_PATH, PLUGIN_FILES_PATH, PLUGIN_IMAGES_PATH] as $dir) {
    if (!is_dir($dir)) mkdir($dir, 0755, true);
}

// Get the route
$route = trim($_GET['route'] ?? '', '/');
$method = $_SERVER['REQUEST_METHOD'];

// Route mapping
switch (true) {
    // === PUBLIC PAGES ===
    case $route === '' || $route === 'home':
        require __DIR__ . '/pages/home.php';
        break;

    case $route === 'plugins':
        require __DIR__ . '/pages/plugins.php';
        break;

    case preg_match('#^plugin/([a-z0-9-]+)$#', $route, $m) === 1:
        $_GET['slug'] = $m[1];
        require __DIR__ . '/pages/plugin-detail.php';
        break;

    case preg_match('#^plugin/(\d+)$#', $route, $m) === 1:
        $_GET['id'] = $m[1];
        require __DIR__ . '/pages/plugin-detail.php';
        break;

    // === AUTH ===
    case $route === 'register':
        require __DIR__ . '/pages/auth/register.php';
        break;

    case $route === 'login':
        require __DIR__ . '/pages/auth/login.php';
        break;

    case $route === 'logout':
        require __DIR__ . '/pages/auth/logout.php';
        break;

    case $route === 'forgot-password':
        require __DIR__ . '/pages/auth/forgot-password.php';
        break;

    case $route === 'reset-password':
        require __DIR__ . '/pages/auth/reset-password.php';
        break;

    // === USER DASHBOARD ===
    case $route === 'dashboard':
        require __DIR__ . '/pages/dashboard.php';
        break;

    case $route === 'profile':
        require __DIR__ . '/pages/profile.php';
        break;

    case $route === 'upgrade-devices':
        require __DIR__ . '/pages/upgrade-devices.php';
        break;

    case preg_match('#^dashboard/deactivate-device/(\d+)$#', $route, $m) === 1:
        $_GET['device_id'] = $m[1];
        require __DIR__ . '/pages/deactivate-device.php';
        break;

    // === CHECKOUT ===
    case preg_match('#^checkout/(\d+)/(\d+)$#', $route, $m) === 1:
        $_GET['plugin_id'] = $m[1];
        $_GET['pricing_id'] = $m[2];
        require __DIR__ . '/pages/checkout.php';
        break;

    case $route === 'checkout/device-upgrade':
        require __DIR__ . '/pages/checkout/device-upgrade.php';
        break;

    case preg_match('#^checkout/success/(\d+)$#', $route, $m) === 1:
        $_GET['order_id'] = $m[1];
        require __DIR__ . '/pages/checkout-success.php';
        break;

    // === DOWNLOADS ===
    case preg_match('#^download/(\d+)/(\d+)$#', $route, $m) === 1:
        $_GET['plugin_id'] = $m[1];
        $_GET['version_id'] = $m[2];
        require __DIR__ . '/pages/download.php';
        break;

    // === API ===
    case $route === 'api/plugin-activate':
        require __DIR__ . '/pages/api/plugin-activate.php';
        break;

    case $route === 'api/plugin-validate':
        require __DIR__ . '/pages/api/plugin-validate.php';
        break;

    case $route === 'api/login':
        require __DIR__ . '/pages/api/login.php';
        break;

    case $route === 'api/verify-license':
        require __DIR__ . '/pages/api/verify-license.php';
        break;

    case $route === 'api/activate-device':
        require __DIR__ . '/pages/api/activate-device.php';
        break;

    case $route === 'api/deactivate-device':
        require __DIR__ . '/pages/api/deactivate-device.php';
        break;

    // === ADMIN ===
    case $route === 'admin':
    case $route === 'admin/dashboard':
        require __DIR__ . '/pages/admin/dashboard.php';
        break;

    case $route === 'admin/plugins':
        require __DIR__ . '/pages/admin/plugins.php';
        break;

    case $route === 'admin/plugins/create':
        require __DIR__ . '/pages/admin/plugin-create.php';
        break;

    case preg_match('#^admin/plugins/edit/(\d+)$#', $route, $m) === 1:
        $_GET['id'] = $m[1];
        require __DIR__ . '/pages/admin/plugin-edit.php';
        break;

    case preg_match('#^admin/plugins/delete/(\d+)$#', $route, $m) === 1:
        $_GET['id'] = $m[1];
        require __DIR__ . '/pages/admin/plugin-delete.php';
        break;

    case preg_match('#^admin/plugins/(\d+)$#', $route, $m) === 1:
        $_GET['id'] = $m[1];
        require __DIR__ . '/pages/admin/plugin-view.php';
        break;

    case preg_match('#^admin/plugins/(\d+)/add-version$#', $route, $m) === 1:
        $_GET['id'] = $m[1];
        require __DIR__ . '/pages/admin/plugin-add-version.php';
        break;

    case preg_match('#^admin/plugins/(\d+)/add-image$#', $route, $m) === 1:
        $_GET['id'] = $m[1];
        require __DIR__ . '/pages/admin/plugin-add-image.php';
        break;

    case preg_match('#^admin/plugins/delete-image/(\d+)$#', $route, $m) === 1:
        $_GET['id'] = $m[1];
        require __DIR__ . '/pages/admin/plugin-delete-image.php';
        break;

    case $route === 'admin/users':
        require __DIR__ . '/pages/admin/users.php';
        break;

    case preg_match('#^admin/users/(\d+)$#', $route, $m) === 1:
        $_GET['id'] = $m[1];
        require __DIR__ . '/pages/admin/user-view.php';
        break;

    case preg_match('#^admin/users/edit/(\d+)$#', $route, $m) === 1:
        $_GET['id'] = $m[1];
        require __DIR__ . '/pages/admin/user-edit.php';
        break;

    case $route === 'admin/orders':
        require __DIR__ . '/pages/admin/orders.php';
        break;

    case preg_match('#^admin/orders/(\d+)$#', $route, $m) === 1:
        $_GET['id'] = $m[1];
        require __DIR__ . '/pages/admin/order-view.php';
        break;

    case preg_match('#^admin/orders/refund/(\d+)$#', $route, $m) === 1:
        $_GET['id'] = $m[1];
        require __DIR__ . '/pages/admin/order-refund.php';
        break;

    case $route === 'admin/licenses':
        require __DIR__ . '/pages/admin/licenses.php';
        break;

    case $route === 'admin/licenses/create':
        require __DIR__ . '/pages/admin/license-create.php';
        break;

    case preg_match('#^admin/licenses/(\d+)$#', $route, $m) === 1:
        $_GET['id'] = $m[1];
        require __DIR__ . '/pages/admin/license-view.php';
        break;

    case preg_match('#^admin/licenses/action/(\d+)$#', $route, $m) === 1:
        $_GET['id'] = $m[1];
        require __DIR__ . '/pages/admin/license-action.php';
        break;

    // === 404 ===
    default:
        http_response_code(404);
        require __DIR__ . '/pages/404.php';
        break;
}
