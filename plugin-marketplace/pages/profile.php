<?php
require_login();
$pageTitle = 'Profile - ' . SITE_NAME;
$user = current_user();
$errors = get_validation_errors();

if ($method === 'POST') {
    require_csrf();
    $action = $_POST['action'] ?? 'update_profile';

    if ($action === 'update_profile') {
        $name = trim($_POST['name'] ?? '');
        $email = trim($_POST['email'] ?? '');
        $errs = [];
        if (empty($name)) $errs['name'] = 'Name is required.';
        if (empty($email) || !filter_var($email, FILTER_VALIDATE_EMAIL)) $errs['email'] = 'Valid email is required.';
        if (empty($errs) && Database::count('users', 'email = ? AND id != ?', [$email, $user['id']]) > 0) {
            $errs['email'] = 'Email is already in use.';
        }
        if (empty($errs)) {
            Database::update('users', ['name' => $name, 'email' => $email], 'id = ?', [$user['id']]);
            $_SESSION['user_name'] = $name;
            set_flash('success', 'Profile updated successfully.');
        } else {
            set_validation_errors($errs);
        }
    } elseif ($action === 'change_password') {
        $current = $_POST['current_password'] ?? '';
        $newPass = $_POST['new_password'] ?? '';
        $confirm = $_POST['confirm_password'] ?? '';
        $errs = [];
        if (!password_verify($current, $user['password'])) $errs['current_password'] = 'Current password is incorrect.';
        if (strlen($newPass) < PASSWORD_MIN_LENGTH) $errs['new_password'] = 'New password must be at least ' . PASSWORD_MIN_LENGTH . ' characters.';
        if ($newPass !== $confirm) $errs['confirm_password'] = 'Passwords do not match.';
        if (empty($errs)) {
            Database::update('users', ['password' => password_hash($newPass, PASSWORD_DEFAULT)], 'id = ?', [$user['id']]);
            set_flash('success', 'Password changed successfully.');
        } else {
            set_validation_errors($errs);
        }
    }
    redirect('/profile');
}

require BASE_PATH . '/includes/header.php';
?>

<div class="max-w-3xl mx-auto px-4 sm:px-6 lg:px-8 py-10">
    <h1 class="text-3xl font-bold text-white mb-8">Profile Settings</h1>

    <!-- Update Profile -->
    <div class="bg-gray-900 border border-gray-800 rounded-2xl p-6 mb-6">
        <h2 class="text-lg font-semibold text-white mb-4">Account Information</h2>
        <form method="POST" action="/profile" class="space-y-4">
            <?= csrf_field() ?>
            <input type="hidden" name="action" value="update_profile">
            <div>
                <label for="name" class="block text-sm font-medium text-gray-300 mb-1.5">Name</label>
                <input type="text" id="name" name="name" value="<?= e($user['name']) ?>" required
                       class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 focus:ring-1 focus:ring-brand-500 transition">
                <?php if (!empty($errors['name'])): ?><p class="mt-1 text-sm text-red-400"><?= e($errors['name']) ?></p><?php endif; ?>
            </div>
            <div>
                <label for="email" class="block text-sm font-medium text-gray-300 mb-1.5">Email</label>
                <input type="email" id="email" name="email" value="<?= e($user['email']) ?>" required
                       class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 focus:ring-1 focus:ring-brand-500 transition">
                <?php if (!empty($errors['email'])): ?><p class="mt-1 text-sm text-red-400"><?= e($errors['email']) ?></p><?php endif; ?>
            </div>
            <div class="flex justify-end">
                <button type="submit" class="px-6 py-2.5 bg-brand-600 hover:bg-brand-500 text-white text-sm font-medium rounded-xl transition">Save Changes</button>
            </div>
        </form>
    </div>

    <!-- Change Password -->
    <div class="bg-gray-900 border border-gray-800 rounded-2xl p-6">
        <h2 class="text-lg font-semibold text-white mb-4">Change Password</h2>
        <form method="POST" action="/profile" class="space-y-4">
            <?= csrf_field() ?>
            <input type="hidden" name="action" value="change_password">
            <div>
                <label for="current_password" class="block text-sm font-medium text-gray-300 mb-1.5">Current Password</label>
                <input type="password" id="current_password" name="current_password" required
                       class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 focus:ring-1 focus:ring-brand-500 transition">
                <?php if (!empty($errors['current_password'])): ?><p class="mt-1 text-sm text-red-400"><?= e($errors['current_password']) ?></p><?php endif; ?>
            </div>
            <div>
                <label for="new_password" class="block text-sm font-medium text-gray-300 mb-1.5">New Password</label>
                <input type="password" id="new_password" name="new_password" required
                       class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 focus:ring-1 focus:ring-brand-500 transition">
                <?php if (!empty($errors['new_password'])): ?><p class="mt-1 text-sm text-red-400"><?= e($errors['new_password']) ?></p><?php endif; ?>
            </div>
            <div>
                <label for="confirm_password" class="block text-sm font-medium text-gray-300 mb-1.5">Confirm New Password</label>
                <input type="password" id="confirm_password" name="confirm_password" required
                       class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white focus:outline-none focus:border-brand-500 focus:ring-1 focus:ring-brand-500 transition">
                <?php if (!empty($errors['confirm_password'])): ?><p class="mt-1 text-sm text-red-400"><?= e($errors['confirm_password']) ?></p><?php endif; ?>
            </div>
            <div class="flex justify-end">
                <button type="submit" class="px-6 py-2.5 bg-brand-600 hover:bg-brand-500 text-white text-sm font-medium rounded-xl transition">Change Password</button>
            </div>
        </form>
    </div>
</div>

<?php require BASE_PATH . '/includes/footer.php'; ?>
