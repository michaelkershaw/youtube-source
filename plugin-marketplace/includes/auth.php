<?php
/**
 * Authentication & Session Helpers
 */

function start_session(): void {
    if (session_status() === PHP_SESSION_NONE) {
        session_start();
    }
}

function is_logged_in(): bool {
    start_session();
    return isset($_SESSION['user_id']);
}

function require_login(): void {
    if (!is_logged_in()) {
        $_SESSION['redirect_after_login'] = $_SERVER['REQUEST_URI'];
        redirect('/login');
    }
}

function require_admin(): void {
    require_login();
    if (!is_admin()) {
        http_response_code(403);
        die('Access denied.');
    }
}

function current_user(): ?array {
    if (!is_logged_in()) return null;
    return Database::fetch("SELECT * FROM users WHERE id = ? AND is_active = 1", [$_SESSION['user_id']]);
}

function current_user_id(): ?int {
    return $_SESSION['user_id'] ?? null;
}

function is_admin(): bool {
    if (!is_logged_in()) return false;
    $user = current_user();
    return $user && $user['role'] === 'admin';
}

function login_user(array $user): void {
    start_session();
    session_regenerate_id(true);
    $_SESSION['user_id'] = $user['id'];
    $_SESSION['user_name'] = $user['name'];
    $_SESSION['user_role'] = $user['role'];
}

function logout_user(): void {
    start_session();
    $_SESSION = [];
    if (ini_get('session.use_cookies')) {
        $params = session_get_cookie_params();
        setcookie(session_name(), '', time() - 42000,
            $params['path'], $params['domain'],
            $params['secure'], $params['httponly']
        );
    }
    session_destroy();
}

// CSRF Protection
function generate_csrf(): string {
    start_session();
    if (empty($_SESSION[CSRF_TOKEN_NAME])) {
        $_SESSION[CSRF_TOKEN_NAME] = bin2hex(random_bytes(32));
    }
    return $_SESSION[CSRF_TOKEN_NAME];
}

function csrf_field(): string {
    return '<input type="hidden" name="' . CSRF_TOKEN_NAME . '" value="' . htmlspecialchars(generate_csrf()) . '">';
}

function verify_csrf(): bool {
    start_session();
    $token = $_POST[CSRF_TOKEN_NAME] ?? '';
    if (empty($token) || empty($_SESSION[CSRF_TOKEN_NAME])) return false;
    return hash_equals($_SESSION[CSRF_TOKEN_NAME], $token);
}

function require_csrf(): void {
    if (!verify_csrf()) {
        http_response_code(403);
        die('Invalid security token. Please go back and try again.');
    }
}

// Flash messages
function set_flash(string $type, string $message): void {
    start_session();
    $_SESSION['flash'][$type] = $message;
}

function get_flash(string $type): ?string {
    start_session();
    $msg = $_SESSION['flash'][$type] ?? null;
    unset($_SESSION['flash'][$type]);
    return $msg;
}

function has_flash(string $type): bool {
    start_session();
    return isset($_SESSION['flash'][$type]);
}
