<?php
if (is_logged_in()) redirect('/dashboard');

$pageTitle = 'Create Account - ' . SITE_NAME;
$errors = get_validation_errors();

if ($method === 'POST') {
    require_csrf();
    $name = trim($_POST['name'] ?? '');
    $email = trim($_POST['email'] ?? '');
    $password = $_POST['password'] ?? '';
    $password_confirm = $_POST['password_confirm'] ?? '';

    $errors = [];
    if (empty($name)) $errors['name'] = 'Name is required.';
    if (empty($email) || !filter_var($email, FILTER_VALIDATE_EMAIL)) $errors['email'] = 'Valid email is required.';
    if (strlen($password) < PASSWORD_MIN_LENGTH) $errors['password'] = 'Password must be at least ' . PASSWORD_MIN_LENGTH . ' characters.';
    if ($password !== $password_confirm) $errors['password_confirm'] = 'Passwords do not match.';

    if (empty($errors) && Database::count('users', 'email = ?', [$email]) > 0) {
        $errors['email'] = 'Email is already registered.';
    }

    if (empty($errors)) {
        $userId = Database::insert('users', [
            'name' => $name,
            'email' => $email,
            'password' => password_hash($password, PASSWORD_DEFAULT),
            'role' => 'user',
            'is_active' => 1,
            'created_at' => date('Y-m-d H:i:s'),
            'updated_at' => date('Y-m-d H:i:s'),
        ]);
        $user = Database::fetch("SELECT * FROM users WHERE id = ?", [$userId]);
        login_user($user);
        set_flash('success', 'Account created successfully! Welcome to ' . SITE_NAME . '.');
        $redirectTo = $_SESSION['redirect_after_login'] ?? '/dashboard';
        unset($_SESSION['redirect_after_login']);
        redirect($redirectTo);
    }

    set_old_input(['name' => $name, 'email' => $email]);
    set_validation_errors($errors);
    redirect('/register');
}

require BASE_PATH . '/includes/header.php';
?>

<div class="min-h-[80vh] flex items-center justify-center px-4 py-16">
    <div class="w-full max-w-md">
        <div class="text-center mb-8">
            <h1 class="text-2xl font-bold text-white">Create your account</h1>
            <p class="text-gray-400 mt-2">Start purchasing and managing plugins today.</p>
        </div>

        <div class="bg-gray-900 border border-gray-800 rounded-2xl p-8">
            <form method="POST" action="/register" class="space-y-5">
                <?= csrf_field() ?>

                <div>
                    <label for="name" class="block text-sm font-medium text-gray-300 mb-1.5">Full Name</label>
                    <input type="text" id="name" name="name" value="<?= old('name') ?>" required autofocus
                           class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white placeholder-gray-500 focus:outline-none focus:border-brand-500 focus:ring-1 focus:ring-brand-500 transition">
                    <?php if (!empty($errors['name'])): ?><p class="mt-1 text-sm text-red-400"><?= e($errors['name']) ?></p><?php endif; ?>
                </div>

                <div>
                    <label for="email" class="block text-sm font-medium text-gray-300 mb-1.5">Email Address</label>
                    <input type="email" id="email" name="email" value="<?= old('email') ?>" required
                           class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white placeholder-gray-500 focus:outline-none focus:border-brand-500 focus:ring-1 focus:ring-brand-500 transition">
                    <?php if (!empty($errors['email'])): ?><p class="mt-1 text-sm text-red-400"><?= e($errors['email']) ?></p><?php endif; ?>
                </div>

                <div>
                    <label for="password" class="block text-sm font-medium text-gray-300 mb-1.5">Password</label>
                    <input type="password" id="password" name="password" required
                           class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white placeholder-gray-500 focus:outline-none focus:border-brand-500 focus:ring-1 focus:ring-brand-500 transition">
                    <p class="mt-1 text-xs text-gray-500">Minimum <?= PASSWORD_MIN_LENGTH ?> characters</p>
                    <?php if (!empty($errors['password'])): ?><p class="mt-1 text-sm text-red-400"><?= e($errors['password']) ?></p><?php endif; ?>
                </div>

                <div>
                    <label for="password_confirm" class="block text-sm font-medium text-gray-300 mb-1.5">Confirm Password</label>
                    <input type="password" id="password_confirm" name="password_confirm" required
                           class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white placeholder-gray-500 focus:outline-none focus:border-brand-500 focus:ring-1 focus:ring-brand-500 transition">
                    <?php if (!empty($errors['password_confirm'])): ?><p class="mt-1 text-sm text-red-400"><?= e($errors['password_confirm']) ?></p><?php endif; ?>
                </div>

                <button type="submit" class="w-full py-3 bg-brand-600 hover:bg-brand-500 text-white font-semibold rounded-xl transition shadow-lg shadow-brand-600/20">
                    Create Account
                </button>
            </form>

            <p class="mt-6 text-center text-sm text-gray-400">
                Already have an account? <a href="/login" class="text-brand-400 hover:text-brand-300 font-medium transition">Sign in</a>
            </p>
        </div>
    </div>
</div>

<?php
clear_old_input();
require BASE_PATH . '/includes/footer.php';
?>
