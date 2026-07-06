<?php if (!defined('BASE_PATH')) exit; ?>
</main>

<!-- Footer -->
<footer class="border-t border-gray-800/50 mt-auto">
    <div class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-12">
        <div class="grid grid-cols-1 md:grid-cols-4 gap-8">
            <div class="col-span-1 md:col-span-2">
                <div class="flex items-center gap-2 mb-4">
                    <div class="w-8 h-8 bg-gradient-to-br from-brand-500 to-purple-600 rounded-lg flex items-center justify-center">
                        <svg class="w-5 h-5 text-white" fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" d="M20 7l-8-4-8 4m16 0l-8 4m8-4v10l-8 4m0-10L4 7m8 4v10M4 7v10l8 4"/></svg>
                    </div>
                    <span class="text-lg font-bold text-white"><?= e(SITE_NAME) ?></span>
                </div>
                <p class="text-gray-400 text-sm max-w-md">The premium marketplace for high-quality plugins. Secure licensing, instant delivery, and automatic updates.</p>
            </div>
            <div>
                <h4 class="text-sm font-semibold text-white mb-4 uppercase tracking-wider">Quick Links</h4>
                <ul class="space-y-2">
                    <li><a href="/plugins" class="text-gray-400 hover:text-brand-400 text-sm transition">Browse Plugins</a></li>
                    <?php if (is_logged_in()): ?>
                        <li><a href="/dashboard" class="text-gray-400 hover:text-brand-400 text-sm transition">Dashboard</a></li>
                    <?php else: ?>
                        <li><a href="/register" class="text-gray-400 hover:text-brand-400 text-sm transition">Create Account</a></li>
                    <?php endif; ?>
                </ul>
            </div>
            <div>
                <h4 class="text-sm font-semibold text-white mb-4 uppercase tracking-wider">Support</h4>
                <ul class="space-y-2">
                    <li><a href="#" class="text-gray-400 hover:text-brand-400 text-sm transition">Documentation</a></li>
                    <li><a href="#" class="text-gray-400 hover:text-brand-400 text-sm transition">Contact Us</a></li>
                    <li><a href="#" class="text-gray-400 hover:text-brand-400 text-sm transition">Terms of Service</a></li>
                </ul>
            </div>
        </div>
        <div class="border-t border-gray-800/50 mt-8 pt-8 text-center">
            <p class="text-gray-500 text-sm">&copy; <?= date('Y') ?> <?= e(SITE_NAME) ?>. All rights reserved.</p>
        </div>
    </div>
</footer>

<script>
// Initialize AOS (Animate On Scroll)
if (typeof AOS !== 'undefined') {
    AOS.init({
        duration: 800,
        easing: 'ease-in-out',
        once: true,
        offset: 100
    });
}
</script>
</body>
</html>
