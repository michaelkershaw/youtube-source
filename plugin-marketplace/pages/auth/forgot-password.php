<?php
if (is_logged_in()) redirect('/dashboard');

$pageTitle = 'Forgot Password - ' . SITE_NAME;
$errors = get_validation_errors();

if ($method === 'POST') {
    require_csrf();
    $email = trim($_POST['email'] ?? '');

    if (empty($email) || !filter_var($email, FILTER_VALIDATE_EMAIL)) {
        set_validation_errors(['email' => 'Valid email is required.']);
        redirect('/forgot-password');
    }

    $user = Database::fetch("SELECT * FROM users WHERE email = ?", [$email]);
    if ($user) {
        $token = bin2hex(random_bytes(32));
        Database::update('users', [
            'reset_token' => hash('sha256', $token),
            'reset_token_expires' => date('Y-m-d H:i:s', strtotime('+1 hour')),
        ], 'id = ?', [$user['id']]);

        // In production, send email with reset link
        // For now, show the token directly (demo mode)
        set_flash('success', 'Password reset link: ' . url('/reset-password?token=' . $token . '&email=' . urlencode($email)));
    } else {
        // Don't reveal if email exists
        set_flash('success', 'If that email exists in our system, a password reset link has been sent.');
    }
    redirect('/forgot-password');
}

require BASE_PATH . '/includes/header.php';
?>

<div class="min-h-[80vh] flex items-center justify-center px-4 py-16">
    <div class="w-full max-w-md">
        <div class="text-center mb-8">
            <h1 class="text-2xl font-bold text-white">Reset your password</h1>
            <p class="text-gray-400 mt-2">Enter your email and we'll send you a reset link.</p>
        </div>

        <div class="bg-gray-900 border border-gray-800 rounded-2xl p-8">
            <form method="POST" action="/forgot-password" class="space-y-5">
                <?= csrf_field() ?>

                <div>
                    <label for="email" class="block text-sm font-medium text-gray-300 mb-1.5">Email Address</label>
                    <input type="email" id="email" name="email" value="<?= old('email') ?>" required autofocus
                           class="w-full px-4 py-2.5 bg-gray-800 border border-gray-700 rounded-xl text-white placeholder-gray-500 focus:outline-none focus:border-brand-500 focus:ring-1 focus:ring-brand-500 transition">
                    <?php if (!empty($errors['email'])): ?><p class="mt-1 text-sm text-red-400"><?= e($errors['email']) ?></p><?php endif; ?>
                </div>

                <button type="submit" class="w-full py-3 bg-brand-600 hover:bg-brand-500 text-white font-semibold rounded-xl transition shadow-lg shadow-brand-600/20">
                    Send Reset Link
                </button>
            </form>

            <p class="mt-6 text-center text-sm text-gray-400">
                Remember your password? <a href="/login" class="text-brand-400 hover:text-brand-300 font-medium transition">Sign in</a>
            </p>
        </div>
    </div>
</div>

<?php
clear_old_input();
require BASE_PATH . '/includes/footer.php';
?>
