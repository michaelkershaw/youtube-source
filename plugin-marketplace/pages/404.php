<?php
$pageTitle = 'Page Not Found - ' . SITE_NAME;
require BASE_PATH . '/includes/header.php';
?>

<div class="min-h-[60vh] flex items-center justify-center px-4 py-16">
    <div class="text-center max-w-md">
        <div class="text-8xl font-extrabold bg-gradient-to-r from-brand-400 to-purple-400 bg-clip-text text-transparent mb-4">404</div>
        <h1 class="text-2xl font-bold text-white mb-3">Page Not Found</h1>
        <p class="text-gray-400 mb-8">The page you're looking for doesn't exist or has been moved.</p>
        <div class="flex flex-col sm:flex-row items-center justify-center gap-4">
            <a href="/" class="px-6 py-3 bg-brand-600 hover:bg-brand-500 text-white font-semibold rounded-xl transition shadow-lg shadow-brand-600/20">Go Home</a>
            <a href="/plugins" class="px-6 py-3 bg-gray-800 hover:bg-gray-700 text-white font-medium rounded-xl transition border border-gray-700">Browse Plugins</a>
        </div>
    </div>
</div>

<?php require BASE_PATH . '/includes/footer.php'; ?>
