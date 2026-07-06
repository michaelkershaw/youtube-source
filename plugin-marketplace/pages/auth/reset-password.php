<?php
if (is_logged_in()) redirect('/dashboard');

$pageTitle = 'Reset Password - ' . SITE_NAME;
$errors = get_validation_errors();
$token = $_GET['token'] ?? '';
$email = $_GET['email'] ?? '';

if ($method === 'POST') {
    require_csrf();
    $token = $_POST['token'] ?? '';
    $email = trim($_POST['email'] ?? '');
    $password = $_POST['password'] ?? '';
    $password_confirm = $_POST['password_confirm'] ?? '';

    $errs = [];
    if (empty($token)) $errs['token'] = 'Invalid reset token.';
    if (empty($email)) $errs['email'] = 'Email is required.';
    if (strlen($password) < PASSWORD_MIN_LENGTH) $errs['password'] = 'Password must be at least ' . PASSWORD_MIN_LENGTH . ' characters.';
    if ($password !== $password_confirm) $errs['password_confirm'] = 'Passwords do not match.';

    if (empty($errs)) {
        $user = Database::fetch(
            "SELECT * FROM users WHERE email = ? AND reset_token = ? AND reset_token_expires > NOW()",
            [$email, hash('sha256', $token)]
        );
        if (!$user) {
            $errs['token'] = 'Invalid or expired reset token.';
        } else {
            Database::update('users', [
                'password' => password_hash($password, PASSWORD_DEFAULT),
                'reset_token' => null,
                'reset_token_expires' => null,
            ], 'id = ?', [$user['id']]);
            set_flash('success', 'Password has been reset. You can now sign in.');
            redirect('/login');
        }
    }
    set_validation_errors($errs);
    redirect('/reset-password?token=' . urlencode($token) . '&email=' . urlencode($email));
}

require BASE_PATH . '/includes/header.php';
?>

<div class="min-h-[80vh] flex items-center justify-center px-4 py-16">
    <div class="w-full max-w-md">
        <div class="text-center mb-8">
            <h1 class="text-2xl font-bold text-white">Set new password</h1>
            <p class="text-gray-400 mt-2">Enter your new password below.</p>
        </div>

        <div class="bg-gray-900 border border-gray-800 rounded-2xl p-8">
            <form method="POST" action="/reset-password" class="space-y-5">
                <?= csrf_field() ?>
                <input type="hidden" name="token" value="<?= e($token) ?>">
                <input type="hidden" name="email" value="<?= e($email) ?>">

                <?php if (!empty($errors['token'])): ?>
                    <div class="bg-red-500/10 border border-red-500/20 text-red-400 text-sm px-4 py-3 rounded-lg"><?= e($errors['token']) ?></div>
                <?php endif; ?>

                <div>
                    <label for="password" class="block text-sm font-medium text-gray-300 mb-1.5">New Password</label>
                    <input type="password" id="password" name="password" required autofocus
                           class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white placeholder-gray-500 focus:outline-none focus:border-brand-500 focus:ring-1 focus:ring-brand-500 transition">
                    <?php if (!empty($errors['password'])): ?><p class="mt-1 text-sm text-red-400"><?= e($errors['password']) ?></p><?php endif; ?>
                </div>

                <div>
                    <label for="password_confirm" class="block text-sm font-medium text-gray-300 mb-1.5">Confirm Password</label>
                    <input type="password" id="password_confirm" name="password_confirm" required
                           class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white placeholder-gray-500 focus:outline-none focus:border-brand-500 focus:ring-1 focus:ring-brand-500 transition">
                    <?php if (!empty($errors['password_confirm'])): ?><p class="mt-1 text-sm text-red-400"><?= e($errors['password_confirm']) ?></p><?php endif; ?>
                </div>

                <button type="submit" class="w-full py-3 bg-brand-600 hover:bg-brand-500 text-white font-semibold rounded-xl transition shadow-lg shadow-brand-600/20">
                    Reset Password
                </button>
            </form>
        </div>
    </div>
</div>

<?php require BASE_PATH . '/includes/footer.php'; ?>
