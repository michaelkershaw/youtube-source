<?php
require_login();
$licenseKey = $_GET['license'] ?? '';
$backUrl = $_GET['back'] ?? '/dashboard';

// Find the license
$license = Database::fetch(
    "SELECT l.*, p.name as plugin_name, p.slug as plugin_slug, u.name as user_name
     FROM licenses l
     JOIN plugins p ON l.plugin_id = p.id
     JOIN users u ON l.user_id = u.id
     WHERE l.license_key = ? AND l.user_id = ? AND l.status = 'active' 
     AND (l.expires_at IS NULL OR l.expires_at > NOW())",
    [$licenseKey, current_user_id()]
);

if (!$license) {
    set_flash('error', 'License not found or not active. Please check your license key.');
    redirect($backUrl);
}

// Get current max activations (use updated value if exists)
$license['current_max_activations'] = $license['max_activations_updated'] ?? $license['max_activations'];

if (!$license) {
    set_flash('error', 'License not found or not active.');
    redirect($backUrl);
}

if (!$license) {
    set_flash('error', 'License not found or not active.');
    redirect($backUrl);
}

// Only lifetime licenses can upgrade
if ($license['license_type'] !== 'lifetime') {
    set_flash('error', 'Device slot upgrades are only available for lifetime licenses.');
    redirect($backUrl);
}

// Get available device slot pricing options
$pricingOptions = Database::fetchAll(
    "SELECT * FROM device_slot_pricing 
     WHERE (plugin_id = ? OR plugin_id IS NULL) AND is_active = 1 
     ORDER BY slots_count ASC",
    [$license['plugin_id']]
);

// Get upgrade history
$upgradeHistory = [];
if ($license['upgrade_history']) {
    $upgradeHistory = json_decode($license['upgrade_history'], true) ?: [];
}

$pageTitle = 'Upgrade Device Slots - ' . SITE_NAME;
require BASE_PATH . '/includes/header.php';
?>

<div class="max-w-4xl mx-auto px-4 sm:px-6 lg:px-8 py-12">
    <!-- Header -->
    <div class="mb-8">
        <a href="<?= e($backUrl) ?>" class="inline-flex items-center gap-2 text-gray-400 hover:text-white transition mb-4">
            <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M15 19l-7-7 7-7"/></svg>
            Back to Dashboard
        </a>
        <h1 class="text-3xl font-bold text-white">Upgrade Device Slots</h1>
        <p class="text-gray-400 mt-2">Add more device slots to your lifetime license</p>
    </div>

    <!-- Current License Info -->
    <div class="bg-gray-900 border border-gray-800 rounded-2xl p-6 mb-8">
        <div class="flex items-center justify-between mb-4">
            <div>current_
                <h2 class="text-xl font-semibold text-white mb-1"><?= e($license['plugin_name']) ?></h2>
                <p class="text-sm text-gray-400">License Key: <code class="bg-gray-800 px-2 py-0.5 rounded"><?= e($license['license_key']) ?></code></p>
            </div>
            <div class="text-right">
                <div class="text-2xl font-bold text-emerald-400"><?= $license['devices'] ?> / <?= $license['current_max_activations'] ?></div>
                <div class="text-xs text-gray-500">Active Devices / Max Slots</div>
            </div>
        </div>
        
        <div class="w-full bg-gray-800 rounded-full h-3">
            <div class="bg-emerald-500 h-3 rounded-full transition-all duration-300" style="width: <?= min(100, ($license['devices'] / $license['current_max_activations']) * 100) ?>%"></div>
        </div>
        
        <div class="mt-4 flex items-center justify-between text-sm">
            <span class="text-gray-400"><?= $license['current_max_activations'] - $license['devices'] ?> slots available</span>
            <span class="text-emerald-400">Lifetime License</span>
        </div>
    </div>

    <!-- Upgrade Options -->
    <div class="bg-gray-900 border border-gray-800 rounded-2xl p-6 mb-8">
        <h3 class="text-lg font-semibold text-white mb-6">Choose Additional Slots</h3>
        
        <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4">
            <?php foreach ($pricingOptions as $option): ?>
            <div class="bg-gray-800 border border-gray-700 rounded-xl p-4 hover:border-brand-500/50 transition-all cursor-pointer group">
                <div class="text-center">
                    <div class="text-3xl font-bold text-brand-400 mb-2">+<?= $option['slots_count'] ?></div>
                    <div class="text-sm text-gray-400 mb-3">Additional Slots</div>
                    <div class="text-2xl font-bold text-white mb-4"><?= format_price($option['price']) ?></div>
                    <button onclick="selectUpgrade(<?= $option['id'] ?>, <?= $option['slots_count'] ?>, '<?= format_price($option['price']) ?>')" 
                            class="w-full px-4 py-2 bg-brand-600 hover:bg-brand-500 text-white text-sm font-medium rounded-lg transition group-hover:scale-105">
                        Select
                    </button>
                </div>
            </div>
            <?php endforeach; ?>
        </div>
    </div>

    <!-- Upgrade History -->
    <?php if (!empty($upgradeHistory)): ?>
    <div class="bg-gray-900 border border-gray-800 rounded-2xl p-6">
        <h3 class="text-lg font-semibold text-white mb-4">Upgrade History</h3>
        <div class="space-y-3">
            <?php foreach ($upgradeHistory as $upgrade): ?>
            <div class="flex items-center justify-between py-3 border-t border-gray-800">
                <div>
                    <div class="text-white font-medium">+<?= $upgrade['slots'] ?> slots added</div>
                    <div class="text-xs text-gray-400"><?= format_date($upgrade['date']) ?></div>
                </div>
                <div class="text-right">
                    <div class="text-emerald-400 font-semibold"><?= format_price($upgrade['price']) ?></div>
                    <div class="text-xs text-gray-400">Completed</div>
                </div>
            </div>
            <?php endforeach; ?>
        </div>
    </div>
    <?php endif; ?>
</div>

<!-- Hidden form for checkout -->
<form id="upgradeForm" method="POST" action="/checkout/device-upgrade" style="display: none;">
    <input type="hidden" name="license_id" value="<?= $license['id'] ?>">
    <input type="hidden" name="pricing_id" id="pricing_id">
    <input type="hidden" name="slots" id="slots">
    <input type="hidden" name="price" id="price">
</form>

<script>
function selectUpgrade(pricingId, slots, price) {
    document.getElementById('pricing_id').value = pricingId;
    document.getElementById('slots').value = slots;
    document.getElementById('price').value = price.replace('$', '');
    document.getElementById('upgradeForm').submit();
}
</script>

<?php require BASE_PATH . '/includes/footer.php'; ?>
