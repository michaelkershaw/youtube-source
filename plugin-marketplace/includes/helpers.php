<?php
/**
 * Helper Functions
 */

function redirect(string $url): void {
    header('Location: ' . $url);
    exit;
}

function e(string $value): string {
    return htmlspecialchars($value, ENT_QUOTES, 'UTF-8');
}

function url(string $path = ''): string {
    return BASE_URL . $path;
}

function asset(string $path): string {
    return BASE_URL . '/assets/' . ltrim($path, '/');
}

function old(string $field, string $default = ''): string {
    return e($_SESSION['old_input'][$field] ?? $default);
}

function set_old_input(array $data): void {
    start_session();
    $_SESSION['old_input'] = $data;
}

function clear_old_input(): void {
    unset($_SESSION['old_input']);
}

function format_price(float $price): string {
    return '$' . number_format($price, 2);
}

function format_date(string $date): string {
    return date('M j, Y', strtotime($date));
}

function format_datetime(string $date): string {
    return date('M j, Y g:i A', strtotime($date));
}

function format_bytes(int $bytes): string {
    $units = ['B', 'KB', 'MB', 'GB'];
    $i = 0;
    while ($bytes >= 1024 && $i < count($units) - 1) {
        $bytes /= 1024;
        $i++;
    }
    return round($bytes, 2) . ' ' . $units[$i];
}

function time_ago(string $datetime): string {
    $now = new DateTime();
    $ago = new DateTime($datetime);
    $diff = $now->diff($ago);
    if ($diff->y > 0) return $diff->y . ' year' . ($diff->y > 1 ? 's' : '') . ' ago';
    if ($diff->m > 0) return $diff->m . ' month' . ($diff->m > 1 ? 's' : '') . ' ago';
    if ($diff->d > 0) return $diff->d . ' day' . ($diff->d > 1 ? 's' : '') . ' ago';
    if ($diff->h > 0) return $diff->h . ' hour' . ($diff->h > 1 ? 's' : '') . ' ago';
    if ($diff->i > 0) return $diff->i . ' minute' . ($diff->i > 1 ? 's' : '') . ' ago';
    return 'just now';
}

function days_until(string $date): int {
    $now = new DateTime();
    $target = new DateTime($date);
    return (int) $now->diff($target)->format('%r%a');
}

function generate_slug(string $text): string {
    $text = strtolower(trim($text));
    $text = preg_replace('/[^a-z0-9-]/', '-', $text);
    $text = preg_replace('/-+/', '-', $text);
    return trim($text, '-');
}

function generate_order_number(): string {
    return 'ORD-' . strtoupper(bin2hex(random_bytes(4))) . '-' . rand(100, 999);
}

function generate_download_token(): string {
    return bin2hex(random_bytes(32));
}

function paginate(int $total, int $perPage, int $currentPage): array {
    $totalPages = max(1, (int) ceil($total / $perPage));
    $currentPage = max(1, min($currentPage, $totalPages));
    $offset = ($currentPage - 1) * $perPage;
    return [
        'total' => $total,
        'per_page' => $perPage,
        'current_page' => $currentPage,
        'total_pages' => $totalPages,
        'offset' => $offset,
    ];
}

function validate_file_extension(string $filename, array $allowed): bool {
    $ext = strtolower(pathinfo($filename, PATHINFO_EXTENSION));
    return in_array($ext, $allowed);
}

function upload_file(array $file, string $destination, array $allowedExtensions, int $maxSize): ?string {
    if ($file['error'] !== UPLOAD_ERR_OK) return null;
    if ($file['size'] > $maxSize) return null;
    if (!validate_file_extension($file['name'], $allowedExtensions)) return null;

    if (!is_dir($destination)) {
        mkdir($destination, 0755, true);
    }

    $ext = strtolower(pathinfo($file['name'], PATHINFO_EXTENSION));
    $filename = bin2hex(random_bytes(16)) . '.' . $ext;
    $filepath = $destination . '/' . $filename;

    if (move_uploaded_file($file['tmp_name'], $filepath)) {
        return $filename;
    }
    return null;
}

function delete_file(string $filepath): void {
    if (file_exists($filepath)) {
        unlink($filepath);
    }
}

function get_validation_errors(): array {
    start_session();
    $errors = $_SESSION['validation_errors'] ?? [];
    unset($_SESSION['validation_errors']);
    return $errors;
}

function set_validation_errors(array $errors): void {
    start_session();
    $_SESSION['validation_errors'] = $errors;
}

function render_pagination(array $pagination, string $baseUrl): string {
    if ($pagination['total_pages'] <= 1) return '';
    
    $html = '<nav class="flex items-center justify-center gap-1 mt-8">';
    
    // Previous
    if ($pagination['current_page'] > 1) {
        $html .= '<a href="' . $baseUrl . '?page=' . ($pagination['current_page'] - 1) . '" class="px-3 py-2 text-sm rounded-lg bg-gray-800 text-gray-300 hover:bg-gray-700 transition">&laquo; Prev</a>';
    }
    
    // Pages
    $start = max(1, $pagination['current_page'] - 2);
    $end = min($pagination['total_pages'], $pagination['current_page'] + 2);
    
    if ($start > 1) {
        $html .= '<a href="' . $baseUrl . '?page=1" class="px-3 py-2 text-sm rounded-lg bg-gray-800 text-gray-300 hover:bg-gray-700 transition">1</a>';
        if ($start > 2) $html .= '<span class="px-2 text-gray-500">...</span>';
    }
    
    for ($i = $start; $i <= $end; $i++) {
        if ($i === $pagination['current_page']) {
            $html .= '<span class="px-3 py-2 text-sm rounded-lg bg-brand-600 text-white font-medium">' . $i . '</span>';
        } else {
            $html .= '<a href="' . $baseUrl . '?page=' . $i . '" class="px-3 py-2 text-sm rounded-lg bg-gray-800 text-gray-300 hover:bg-gray-700 transition">' . $i . '</a>';
        }
    }
    
    if ($end < $pagination['total_pages']) {
        if ($end < $pagination['total_pages'] - 1) $html .= '<span class="px-2 text-gray-500">...</span>';
        $html .= '<a href="' . $baseUrl . '?page=' . $pagination['total_pages'] . '" class="px-3 py-2 text-sm rounded-lg bg-gray-800 text-gray-300 hover:bg-gray-700 transition">' . $pagination['total_pages'] . '</a>';
    }
    
    // Next
    if ($pagination['current_page'] < $pagination['total_pages']) {
        $html .= '<a href="' . $baseUrl . '?page=' . ($pagination['current_page'] + 1) . '" class="px-3 py-2 text-sm rounded-lg bg-gray-800 text-gray-300 hover:bg-gray-700 transition">Next &raquo;</a>';
    }
    
    $html .= '</nav>';
    return $html;
}
