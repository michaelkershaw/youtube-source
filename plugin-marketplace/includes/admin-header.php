<?php
if (!defined('BASE_PATH')) exit;
require_admin();
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title><?= e($pageTitle ?? 'Admin - ' . SITE_NAME) ?></title>
    <script src="https://cdn.tailwindcss.com"></script>
    <script>
        tailwind.config = {
            theme: {
                extend: {
                    colors: {
                        brand: { 50:'#eef2ff',100:'#e0e7ff',200:'#c7d2fe',300:'#a5b4fc',400:'#818cf8',500:'#6366f1',600:'#4f46e5',700:'#4338ca',800:'#3730a3',900:'#312e81',950:'#1e1b4b' }
                    },
                    fontFamily: { sans: ['Inter','system-ui','sans-serif'] }
                }
            }
        }
    </script>
    <link rel="preconnect" href="https://fonts.bunny.net">
    <link href="https://fonts.bunny.net/css?family=inter:300,400,500,600,700,800&display=swap" rel="stylesheet"/>
    <style>[x-cloak]{display:none!important;}</style>
    <script defer src="https://cdn.jsdelivr.net/npm/alpinejs@3.x.x/dist/cdn.min.js"></script>
</head>
<body class="bg-gray-950 text-gray-100 font-sans antialiased">
<div x-data="{ sidebarOpen: false }" class="flex min-h-screen">

<!-- Sidebar -->
<aside :class="sidebarOpen ? 'translate-x-0' : '-translate-x-full'" class="fixed inset-y-0 left-0 z-50 w-64 bg-gray-900 border-r border-gray-800 transform transition-transform lg:translate-x-0 lg:static lg:inset-0 flex flex-col">
    <div class="flex items-center gap-2.5 px-6 h-16 border-b border-gray-800 flex-shrink-0">
        <div class="w-8 h-8 bg-gradient-to-br from-brand-500 to-purple-600 rounded-lg flex items-center justify-center">
            <svg class="w-5 h-5 text-white" fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" d="M20 7l-8-4-8 4m16 0l-8 4m8-4v10l-8 4m0-10L4 7m8 4v10M4 7v10l8 4"/></svg>
        </div>
        <span class="text-lg font-bold text-white">Admin</span>
    </div>
    <nav class="flex-1 px-3 py-4 space-y-1 overflow-y-auto">
        <?php
        $adminLinks = [
            ['url' => '/admin', 'label' => 'Dashboard', 'icon' => '<path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M4 6a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2H6a2 2 0 01-2-2V6zm10 0a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2h-2a2 2 0 01-2-2V6zM4 16a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2H6a2 2 0 01-2-2v-2zm10 0a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2h-2a2 2 0 01-2-2v-2z"/>', 'match' => 'admin/dashboard'],
            ['url' => '/admin/plugins', 'label' => 'Plugins', 'icon' => '<path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M20 7l-8-4-8 4m16 0l-8 4m8-4v10l-8 4m0-10L4 7m8 4v10M4 7v10l8 4"/>', 'match' => 'admin/plugin'],
            ['url' => '/admin/users', 'label' => 'Users', 'icon' => '<path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 4.354a4 4 0 110 5.292M15 21H3v-1a6 6 0 0112 0v1zm0 0h6v-1a6 6 0 00-9-5.197M13 7a4 4 0 11-8 0 4 4 0 018 0z"/>', 'match' => 'admin/user'],
            ['url' => '/admin/orders', 'label' => 'Orders', 'icon' => '<path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M9 5H7a2 2 0 00-2 2v12a2 2 0 002 2h10a2 2 0 002-2V7a2 2 0 00-2-2h-2M9 5a2 2 0 002 2h2a2 2 0 002-2M9 5a2 2 0 012-2h2a2 2 0 012 2"/>', 'match' => 'admin/order'],
            ['url' => '/admin/licenses', 'label' => 'Licenses', 'icon' => '<path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M15 7a2 2 0 012 2m4 0a6 6 0 01-7.743 5.743L11 17H9v2H7v2H4a1 1 0 01-1-1v-2.586a1 1 0 01.293-.707l5.964-5.964A6 6 0 1121 9z"/>', 'match' => 'admin/license'],
        ];
        foreach ($adminLinks as $link):
            $active = ($route === trim($link['url'], '/')) || str_starts_with($route ?? '', $link['match']);
        ?>
        <a href="<?= $link['url'] ?>" class="flex items-center gap-3 px-3 py-2.5 rounded-lg text-sm font-medium transition <?= $active ? 'bg-brand-600/10 text-brand-400' : 'text-gray-400 hover:text-white hover:bg-gray-800/50' ?>">
            <svg class="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24"><?= $link['icon'] ?></svg>
            <?= $link['label'] ?>
        </a>
        <?php endforeach; ?>
    </nav>
    <div class="px-3 py-4 border-t border-gray-800 flex-shrink-0">
        <a href="/" class="flex items-center gap-3 px-3 py-2.5 rounded-lg text-sm font-medium text-gray-400 hover:text-white hover:bg-gray-800/50 transition">
            <svg class="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M10 19l-7-7m0 0l7-7m-7 7h18"/></svg>
            Back to Site
        </a>
    </div>
</aside>

<!-- Mobile Overlay -->
<div x-show="sidebarOpen" x-cloak @click="sidebarOpen = false" class="fixed inset-0 bg-black/50 z-40 lg:hidden"></div>

<!-- Main Content -->
<div class="flex-1 flex flex-col min-w-0">
    <!-- Top Bar -->
    <header class="h-16 bg-gray-900/50 border-b border-gray-800 flex items-center justify-between px-4 sm:px-6 flex-shrink-0">
        <button @click="sidebarOpen = !sidebarOpen" class="lg:hidden p-2 text-gray-400 hover:text-white rounded-lg hover:bg-gray-800">
            <svg class="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M4 6h16M4 12h16M4 18h16"/></svg>
        </button>
        <div class="flex items-center gap-3 ml-auto">
            <span class="text-sm text-gray-400"><?= e($_SESSION['user_name'] ?? 'Admin') ?></span>
            <a href="/logout" class="text-sm text-red-400 hover:text-red-300 transition">Sign Out</a>
        </div>
    </header>

    <!-- Flash Messages -->
    <?php if ($msg = get_flash('success')): ?>
    <div x-data="{show:true}" x-show="show" x-init="setTimeout(()=>show=false,5000)" class="mx-4 sm:mx-6 mt-4">
        <div class="bg-emerald-500/10 border border-emerald-500/30 text-emerald-400 px-4 py-3 rounded-xl text-sm flex items-center justify-between">
            <span><?= e($msg) ?></span><button @click="show=false" class="text-emerald-400/60 hover:text-emerald-400">&times;</button>
        </div>
    </div>
    <?php endif; ?>
    <?php if ($msg = get_flash('error')): ?>
    <div x-data="{show:true}" x-show="show" x-init="setTimeout(()=>show=false,6000)" class="mx-4 sm:mx-6 mt-4">
        <div class="bg-red-500/10 border border-red-500/30 text-red-400 px-4 py-3 rounded-xl text-sm flex items-center justify-between">
            <span><?= e($msg) ?></span><button @click="show=false" class="text-red-400/60 hover:text-red-400">&times;</button>
        </div>
    </div>
    <?php endif; ?>

    <!-- Page Content -->
    <main class="flex-1 p-4 sm:p-6 overflow-y-auto">
