<?php
if (is_logged_in()) redirect('/dashboard');

$pageTitle = 'Sign In - ' . SITE_NAME;
$errors = get_validation_errors();

if ($method === 'POST') {
    require_csrf();
    $email = trim($_POST['email'] ?? '');
    $password = $_POST['password'] ?? '';

    $errors = [];
    if (empty($email)) $errors['email'] = 'Email is required.';
    if (empty($password)) $errors['password'] = 'Password is required.';

    if (empty($errors)) {
        $user = Database::fetch("SELECT * FROM users WHERE email = ?", [$email]);
        if (!$user || !password_verify($password, $user['password'])) {
            $errors['email'] = 'Invalid email or password.';
        } elseif (!$user['is_active']) {
            $errors['email'] = 'Your account has been disabled.';
        } else {
            login_user($user);
            set_flash('success', 'Welcome back, ' . $user['name'] . '!');
            $redirectTo = $_SESSION['redirect_after_login'] ?? '/dashboard';
            unset($_SESSION['redirect_after_login']);
            redirect($redirectTo);
        }
    }

    set_old_input(['email' => $email]);
    set_validation_errors($errors);
    redirect('/login');
}

require BASE_PATH . '/includes/header.php';
?>

<div class="min-h-[80vh] flex items-center justify-center px-4 py-16">
    <div class="w-full max-w-md">
        <div class="text-center mb-8">
            <h1 class="text-2xl font-bold text-white">Welcome back</h1>
            <p class="text-gray-400 mt-2">Sign in to your account to continue.</p>
        </div>

        <div class="bg-gray-900 border border-gray-800 rounded-2xl p-8">
            <form method="POST" action="/login" class="space-y-5">
                <?= csrf_field() ?>

                <div>
                    <label for="email" class="block text-sm font-medium text-gray-300 mb-1.5">Email Address</label>
                    <input type="email" id="email" name="email" value="<?= old('email') ?>" required autofocus
                           class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white placeholder-gray-500 focus:outline-none focus:border-brand-500 focus:ring-1 focus:ring-brand-500 transition">
                    <?php if (!empty($errors['email'])): ?><p class="mt-1 text-sm text-red-400"><?= e($errors['email']) ?></p><?php endif; ?>
                </div>

                <div>
                    <div class="flex items-center justify-between mb-1.5">
                        <label for="password" class="block text-sm font-medium text-gray-300">Password</label>
                        <a href="/forgot-password" class="text-xs text-brand-400 hover:text-brand-300 transition">Forgot password?</a>
                    </div>
                    <input type="password" id="password" name="password" required
                           class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white placeholder-gray-500 focus:outline-none focus:border-brand-500 focus:ring-1 focus:ring-brand-500 transition">
                    <?php if (!empty($errors['password'])): ?><p class="mt-1 text-sm text-red-400"><?= e($errors['password']) ?></p><?php endif; ?>
                </div>

                <button type="submit" class="w-full py-3 bg-brand-600 hover:bg-brand-500 text-white font-semibold rounded-xl transition shadow-lg shadow-brand-600/20">
                    Sign In
                </button>
            </form>

            <p class="mt-6 text-center text-sm text-gray-400">
                Don't have an account? <a href="/register" class="text-brand-400 hover:text-brand-300 font-medium transition">Create one</a>
            </p>
        </div>
    </div>
</div>

<?php
clear_old_input();
require BASE_PATH . '/includes/footer.php';
?>
