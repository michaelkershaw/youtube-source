<?php
$pageTitle = 'Edit User - ' . SITE_NAME;
$id = (int)($_GET['id'] ?? 0);
$user = Database::fetch("SELECT * FROM users WHERE id = ?", [$id]);
if (!$user) { set_flash('error', 'User not found.'); redirect('/admin/users'); }

$errors = get_validation_errors();

if ($method === 'POST') {
    require_csrf();
    $name = trim($_POST['name'] ?? '');
    $email = trim($_POST['email'] ?? '');
    $role = $_POST['role'] ?? 'user';
    $isActive = isset($_POST['is_active']) ? 1 : 0;

    $errs = [];
    if (empty($name)) $errs['name'] = 'Name is required.';
    if (empty($email) || !filter_var($email, FILTER_VALIDATE_EMAIL)) $errs['email'] = 'Valid email is required.';
    if (!in_array($role, ['user', 'admin'])) $errs['role'] = 'Invalid role.';
    if (Database::count('users', 'email = ? AND id != ?', [$email, $id]) > 0) $errs['email'] = 'Email already in use.';

    if (!empty($errs)) {
        set_validation_errors($errs);
        redirect('/admin/users/edit/' . $id);
    }

    Database::update('users', [
        'name' => $name,
        'email' => $email,
        'role' => $role,
        'is_active' => $isActive,
        'updated_at' => date('Y-m-d H:i:s'),
    ], 'id = ?', [$id]);

    set_flash('success', 'User updated successfully.');
    redirect('/admin/users');
}

require BASE_PATH . '/includes/admin-header.php';
?>

<div class="mb-6">
    <a href="/admin/users" class="text-sm text-gray-400 hover:text-white transition flex items-center gap-1 mb-2">
        <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M10 19l-7-7m0 0l7-7m-7 7h18"/></svg> Back to Users
    </a>
    <h1 class="text-2xl font-bold text-white">Edit User: <?= e($user['name']) ?></h1>
</div>

<form method="POST" action="/admin/users/edit/<?= $id ?>" class="space-y-6 max-w-2xl">
    <?= csrf_field() ?>
    <div class="bg-gray-900 border border-gray-800 rounded-2xl p-6 space-y-4">
        <div>
            <label class="block text-sm font-medium text-gray-300 mb-1.5">Name *</label>
            <input type="text" name="name" value="<?= e($user['name']) ?>" required class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition">
            <?php if (!empty($errors['name'])): ?><p class="mt-1 text-sm text-red-400"><?= e($errors['name']) ?></p><?php endif; ?>
        </div>
        <div>
            <label class="block text-sm font-medium text-gray-300 mb-1.5">Email *</label>
            <input type="email" name="email" value="<?= e($user['email']) ?>" required class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition">
            <?php if (!empty($errors['email'])): ?><p class="mt-1 text-sm text-red-400"><?= e($errors['email']) ?></p><?php endif; ?>
        </div>
        <div>
            <label class="block text-sm font-medium text-gray-300 mb-1.5">Role</label>
            <select name="role" class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 transition">
                <option value="user" <?= $user['role'] === 'user' ? 'selected' : '' ?>>User</option>
                <option value="admin" <?= $user['role'] === 'admin' ? 'selected' : '' ?>>Admin</option>
            </select>
        </div>
        <label class="flex items-center gap-2 cursor-pointer">
            <input type="checkbox" name="is_active" value="1" <?= $user['is_active'] ? 'checked' : '' ?> class="w-4 h-4 rounded border-gray-600 bg-gray-800 text-brand-600 focus:ring-brand-500">
            <span class="text-sm text-gray-300">Account Active</span>
        </label>
    </div>
    <div class="flex items-center gap-4">
        <button type="submit" class="px-8 py-3 bg-brand-600 hover:bg-brand-500 text-white font-semibold rounded-xl transition">Update User</button>
        <a href="/admin/users" class="px-8 py-3 bg-gray-800 hover:bg-gray-700 text-gray-300 font-medium rounded-xl transition">Cancel</a>
    </div>
</form>

<?php require BASE_PATH . '/includes/admin-footer.php'; ?>
