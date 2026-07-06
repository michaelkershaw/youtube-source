<?php if (!defined('BASE_PATH')) exit; ?>
<!DOCTYPE html>
<html lang="en" class="scroll-smooth">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title><?= e($pageTitle ?? SITE_NAME) ?></title>
    <script src="https://cdn.tailwindcss.com"></script>
    <link href="https://fonts.bunny.net/css?family=inter:400,500,600,700,800,900&display=swap" rel="stylesheet"/>
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.0/css/all.min.css">
    <link href="https://unpkg.com/aos@2.3.1/dist/aos.css" rel="stylesheet">
    <script src="https://unpkg.com/aos@2.3.1/dist/aos.js"></script>
    <style>
        /* TellyMedia Theme Colors */
        :root {
            --red: #ff4500;
            --blue: #00aaff;
            --dark: #0a0a0a;
            --darker: #000000;
            --light: #1a1a1a;
            --gray: #666;
            --text: #e0e0e0;
            --border: #222;
        }
        
        /* Hero Section */
        .hero {
            position: relative;
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            text-align: center;
            overflow: hidden;
            background: var(--darker);
        }

        .hero-video {
            position: absolute;
            top: 50%;
            left: 58%;
            width: 80%;
            height: 80%;
            transform: translate(-50%, -50%);
            z-index: 0;
            object-fit: contain;
        }

        .particles-container {
            position: absolute;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            z-index: 1;
            pointer-events: none;
            overflow: hidden;
        }

        .particle {
            position: absolute;
            width: 2px;
            height: 2px;
            background: white;
            border-radius: 50%;
            box-shadow: 0 0 3px rgba(255, 255, 255, 0.8);
        }

        .particle::before {
            content: '';
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            width: 100%;
            height: 100%;
            background: radial-gradient(circle, rgba(255, 255, 255, 1), transparent);
            border-radius: 50%;
            animation: twinkle 3s infinite ease-in-out;
        }

        @keyframes twinkle {
            0%, 100% { opacity: 0.3; transform: translate(-50%, -50%) scale(1); }
            50% { opacity: 1; transform: translate(-50%, -50%) scale(1.5); }
        }

        .particle-1 { top: 10%; left: 5%; animation: float-star 20s infinite ease-in-out; }
        .particle-2 { top: 20%; left: 15%; animation: float-star 18s infinite ease-in-out 1s; }
        .particle-3 { top: 15%; left: 25%; animation: float-star 22s infinite ease-in-out 2s; }
        .particle-4 { top: 30%; left: 35%; animation: float-star 19s infinite ease-in-out 3s; }
        .particle-5 { top: 25%; left: 45%; animation: float-star 21s infinite ease-in-out 4s; }
        .particle-6 { top: 35%; left: 55%; animation: float-star 17s infinite ease-in-out 5s; }
        .particle-7 { top: 40%; left: 65%; animation: float-star 20s infinite ease-in-out 6s; }
        .particle-8 { top: 45%; left: 75%; animation: float-star 18s infinite ease-in-out 7s; }
        .particle-9 { top: 50%; left: 85%; animation: float-star 23s infinite ease-in-out 8s; }
        .particle-10 { top: 55%; left: 95%; animation: float-star 19s infinite ease-in-out 9s; }
        .particle-11 { top: 60%; left: 10%; animation: float-star 21s infinite ease-in-out 10s; }
        .particle-12 { top: 65%; left: 20%; animation: float-star 18s infinite ease-in-out 11s; }
        .particle-13 { top: 70%; left: 30%; animation: float-star 20s infinite ease-in-out 12s; }
        .particle-14 { top: 75%; left: 40%; animation: float-star 22s infinite ease-in-out 13s; }
        .particle-15 { top: 80%; left: 50%; animation: float-star 19s infinite ease-in-out 14s; }
        .particle-16 { top: 85%; left: 60%; animation: float-star 21s infinite ease-in-out 15s; }
        .particle-17 { top: 90%; left: 70%; animation: float-star 18s infinite ease-in-out 16s; }
        .particle-18 { top: 12%; left: 80%; animation: float-star 20s infinite ease-in-out 17s; }
        .particle-19 { top: 18%; left: 90%; animation: float-star 23s infinite ease-in-out 18s; }
        .particle-20 { top: 22%; left: 12%; animation: float-star 19s infinite ease-in-out 19s; }

        .particle-1, .particle-5, .particle-9, .particle-13, .particle-17 {
            background: #ff4500;
            box-shadow: 0 0 4px rgba(255, 69, 0, 0.9), 0 0 8px rgba(255, 69, 0, 0.5);
        }

        .particle-3, .particle-7, .particle-11, .particle-15, .particle-19 {
            background: #00aaff;
            box-shadow: 0 0 4px rgba(0, 170, 255, 0.9), 0 0 8px rgba(0, 170, 255, 0.5);
        }

        @keyframes float-star {
            0%, 100% { transform: translateY(0) translateX(0); opacity: 0.6; }
            25% { transform: translateY(-10px) translateX(5px); opacity: 1; }
            50% { transform: translateY(-5px) translateX(-5px); opacity: 0.8; }
            75% { transform: translateY(-15px) translateX(3px); opacity: 0.9; }
        }

        .hero-overlay {
            position: absolute;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background: linear-gradient(135deg, rgba(0, 0, 0, 0.4) 0%, rgba(10, 10, 10, 0.3) 50%, rgba(0, 0, 0, 0.4) 100%);
            z-index: 1;
        }

        .hero-overlay::before {
            content: '';
            position: absolute;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background: 
                radial-gradient(ellipse at 20% 30%, rgba(255, 69, 0, 0.1) 0%, transparent 50%),
                radial-gradient(ellipse at 80% 70%, rgba(0, 170, 255, 0.1) 0%, transparent 50%);
            animation: bgPulse 8s ease-in-out infinite;
        }

        @keyframes bgPulse {
            0%, 100% { opacity: 0.6; }
            50% { opacity: 1; }
        }

        .hero-content {
            position: relative;
            z-index: 2;
            padding: 2rem 0;
        }

        .hero-logo {
            max-width: 320px;
            height: auto;
            margin-bottom: 2rem;
            filter: drop-shadow(0 0 30px rgba(255, 69, 0, 0.4)) drop-shadow(0 0 30px rgba(0, 170, 255, 0.4));
            animation: logoFloat 6s ease-in-out infinite;
        }

        @keyframes logoFloat {
            0%, 100% { transform: translateY(0); }
            50% { transform: translateY(-15px); }
        }

        .hero-title {
            font-size: 4.5rem;
            font-weight: 900;
            line-height: 1.1;
            margin-bottom: 1.5rem;
            text-transform: uppercase;
            letter-spacing: 2px;
        }

        .title-telly {
            color: var(--red);
            text-shadow: 0 0 40px rgba(255, 69, 0, 0.6);
        }

        .title-media {
            color: var(--blue);
            text-shadow: 0 0 40px rgba(0, 170, 255, 0.6);
        }

        .hero-subtitle {
            font-size: 1.25rem;
            color: var(--gray);
            margin-bottom: 3rem;
        }

        .hero-cta {
            display: flex;
            gap: 1.5rem;
            justify-content: center;
            margin-bottom: 4rem;
            flex-wrap: wrap;
        }

        .btn-hero {
            display: inline-block;
            padding: 1.25rem 3rem;
            background: linear-gradient(90deg, var(--red) 0%, var(--blue) 100%);
            color: white;
            text-decoration: none;
            border-radius: 50px;
            font-weight: 800;
            font-size: 1rem;
            text-transform: uppercase;
            letter-spacing: 1.5px;
            transition: all 0.3s;
            box-shadow: 0 10px 30px rgba(255, 69, 0, 0.3);
        }

        .btn-hero:hover {
            transform: translateY(-3px);
            box-shadow: 0 15px 40px rgba(255, 69, 0, 0.5), 0 15px 40px rgba(0, 170, 255, 0.3);
        }

        .btn-secondary {
            display: inline-block;
            padding: 1.25rem 3rem;
            background: transparent;
            color: var(--text);
            text-decoration: none;
            border: 2px solid var(--blue);
            border-radius: 50px;
            font-weight: 700;
            font-size: 1rem;
            text-transform: uppercase;
            letter-spacing: 1.5px;
            transition: all 0.3s;
        }

        .btn-secondary:hover {
            background: var(--blue);
            transform: translateY(-3px);
            box-shadow: 0 10px 30px rgba(0, 170, 255, 0.4);
        }

        .hero-stats {
            display: flex;
            gap: 4rem;
            justify-content: center;
            flex-wrap: wrap;
        }

        .stat-box {
            text-align: center;
        }

        .stat-number {
            font-size: 2.5rem;
            font-weight: 900;
            color: white;
            margin-bottom: 0.5rem;
        }

        .stat-label {
            font-size: 0.875rem;
            color: var(--gray);
            text-transform: uppercase;
            letter-spacing: 1px;
        }

        .container {
            max-width: 1200px;
            margin: 0 auto;
            padding: 0 20px;
        }

        .features {
            padding: 6rem 0;
            background: var(--dark);
        }

        .feature-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 2rem;
        }

        .feature-box {
            text-align: center;
            padding: 2rem;
            background: var(--light);
            border-radius: 1rem;
            border: 1px solid var(--border);
            transition: transform 0.3s;
        }

        .feature-box:hover {
            transform: translateY(-5px);
        }

        .feature-icon {
            font-size: 3rem;
            margin-bottom: 1rem;
        }

        .feature-title {
            font-size: 1.25rem;
            font-weight: 700;
            color: white;
            margin-bottom: 1rem;
        }

        .feature-desc {
            color: var(--gray);
            line-height: 1.6;
        }

        .section-title-center {
            text-align: center;
            font-size: 3rem;
            font-weight: 900;
            margin-bottom: 1rem;
            text-transform: uppercase;
            letter-spacing: 2px;
        }

        .section-subtitle {
            text-align: center;
            color: var(--gray);
            margin-bottom: 3rem;
            font-size: 1.125rem;
        }
    </style>
    <link rel="preconnect" href="https://fonts.bunny.net">
    <link href="https://fonts.bunny.net/css?family=inter:300,400,500,600,700,800&display=swap" rel="stylesheet"/>
    <style>
        [x-cloak] { display: none !important; }
        .glass { background: rgba(17,24,39,0.7); backdrop-filter: blur(16px); }
    </style>
    <script defer src="https://cdn.jsdelivr.net/npm/alpinejs@3.x.x/dist/cdn.min.js"></script>
</head>
<body class="bg-gray-950 text-gray-100 font-sans antialiased min-h-screen flex flex-col">

<!-- Navigation -->
<nav x-data="{ mobileOpen: false, profileOpen: false }" style="background: rgba(0, 0, 0, 0.98); backdrop-filter: blur(20px); border-bottom: 1px solid rgba(255, 255, 255, 0.05); position: sticky; top: 0; z-index: 1000; box-shadow: 0 8px 32px rgba(0, 0, 0, 0.8);">
    <div class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div class="flex items-center justify-between h-24">
            <div class="flex items-center gap-8">
                <a href="/" class="flex items-center gap-3 group" style="text-decoration: none;">
                    <div style="width: 55px; height: 55px; background: linear-gradient(135deg, var(--red), var(--blue)); border-radius: 14px; display: flex; align-items: center; justify-content: center; box-shadow: 0 10px 30px rgba(255, 69, 0, 0.4), 0 10px 30px rgba(0, 170, 255, 0.2); transition: all 0.4s;" onmouseover="this.style.transform='translateY(-3px) scale(1.05)'; this.style.boxShadow='0 15px 40px rgba(255, 69, 0, 0.6), 0 15px 40px rgba(0, 170, 255, 0.4)';" onmouseout="this.style.transform='translateY(0) scale(1)'; this.style.boxShadow='0 10px 30px rgba(255, 69, 0, 0.4), 0 10px 30px rgba(0, 170, 255, 0.2)';">
                        <span style="font-size: 1.75rem; filter: drop-shadow(0 0 15px rgba(255, 255, 255, 0.7));">🎵</span>
                    </div>
                    <div class="hidden sm:block">
                        <div style="font-size: 1.35rem; font-weight: 900; letter-spacing: 2px; line-height: 1.2; text-transform: uppercase;">
                            <span style="color: var(--red); text-shadow: 0 0 20px rgba(255, 69, 0, 0.5);">MRK</span>
                        </div>
                        <div style="font-size: 0.75rem; color: #ccc; letter-spacing: 2px; margin-top: 2px; font-weight: 600; line-height: 1.3;">
                            Virtual DJ Plugins
                        </div>
                    </div>
                </a>
                <div class="hidden md:flex items-center gap-2">
                    <a href="/" style="padding: 0.75rem 1.25rem; font-size: 0.95rem; font-weight: 600; border-radius: 10px; transition: all 0.3s; text-decoration: none; <?= ($route ?? '') === '' || ($route ?? '') === 'home' ? 'color: white; background: linear-gradient(135deg, rgba(255, 69, 0, 0.2), rgba(0, 170, 255, 0.2)); border: 1px solid var(--border);' : 'color: var(--gray); border: 1px solid transparent;' ?>" onmouseover="if(!'<?= ($route ?? '') === '' || ($route ?? '') === 'home' ? 'active' : '' ?>') this.style.color='white'; this.style.background='rgba(255, 255, 255, 0.05)';" onmouseout="if(!'<?= ($route ?? '') === '' || ($route ?? '') === 'home' ? 'active' : '' ?>') this.style.color='var(--gray)'; this.style.background='transparent';">
                        <i class="fas fa-home" style="margin-right: 0.5rem;"></i>Home
                    </a>
                    <a href="/plugins" style="padding: 0.75rem 1.25rem; font-size: 0.95rem; font-weight: 600; border-radius: 10px; transition: all 0.3s; text-decoration: none; <?= str_starts_with($route ?? '', 'plugin') ? 'color: white; background: linear-gradient(135deg, rgba(255, 69, 0, 0.2), rgba(0, 170, 255, 0.2)); border: 1px solid var(--border);' : 'color: var(--gray); border: 1px solid transparent;' ?>" onmouseover="if(!'<?= str_starts_with($route ?? '', 'plugin') ? 'active' : '' ?>') this.style.color='white'; this.style.background='rgba(255, 255, 255, 0.05)';" onmouseout="if(!'<?= str_starts_with($route ?? '', 'plugin') ? 'active' : '' ?>') this.style.color='var(--gray)'; this.style.background='transparent';">
                        <i class="fas fa-th-large" style="margin-right: 0.5rem;"></i>Plugins
                    </a>
                    <?php if (is_logged_in()): ?>
                        <a href="/dashboard" style="padding: 0.75rem 1.25rem; font-size: 0.95rem; font-weight: 600; border-radius: 10px; transition: all 0.3s; text-decoration: none; <?= ($route ?? '') === 'dashboard' ? 'color: white; background: linear-gradient(135deg, rgba(255, 69, 0, 0.2), rgba(0, 170, 255, 0.2)); border: 1px solid var(--border);' : 'color: var(--gray); border: 1px solid transparent;' ?>" onmouseover="if(!'<?= ($route ?? '') === 'dashboard' ? 'active' : '' ?>') this.style.color='white'; this.style.background='rgba(255, 255, 255, 0.05)';" onmouseout="if(!'<?= ($route ?? '') === 'dashboard' ? 'active' : '' ?>') this.style.color='var(--gray)'; this.style.background='transparent';">
                            <i class="fas fa-tachometer-alt" style="margin-right: 0.5rem;"></i>Dashboard
                        </a>
                        <?php if (is_admin()): ?>
                            <a href="/admin" style="padding: 0.75rem 1.25rem; font-size: 0.95rem; font-weight: 600; border-radius: 10px; transition: all 0.3s; text-decoration: none; <?= str_starts_with($route ?? '', 'admin') ? 'color: var(--red); background: rgba(255, 69, 0, 0.15); border: 1px solid var(--red);' : 'color: var(--red); border: 1px solid transparent; opacity: 0.7;' ?>" onmouseover="this.style.opacity='1'; this.style.background='rgba(255, 69, 0, 0.1)';" onmouseout="if(!'<?= str_starts_with($route ?? '', 'admin') ? 'active' : '' ?>') this.style.opacity='0.7'; this.style.background='transparent';">
                                <i class="fas fa-cog" style="margin-right: 0.5rem;"></i>Admin
                            </a>
                        <?php endif; ?>
                    <?php endif; ?>
                </div>
            </div>
            <div class="flex items-center gap-3">
                <?php if (!is_logged_in()): ?>
                    <a href="/login" style="padding: 0.75rem 1.5rem; font-size: 0.95rem; font-weight: 600; color: var(--text); transition: all 0.3s; text-decoration: none; border-radius: 10px; border: 1px solid transparent;" class="hidden sm:block" onmouseover="this.style.color='white'; this.style.background='rgba(255, 255, 255, 0.05)';" onmouseout="this.style.color='var(--text)'; this.style.background='transparent';">
                        Sign In
                    </a>
                    <a href="/register" style="padding: 0.75rem 1.75rem; font-size: 0.95rem; font-weight: 800; color: white; background: linear-gradient(90deg, var(--red) 0%, var(--blue) 100%); border-radius: 50px; transition: all 0.3s; text-decoration: none; box-shadow: 0 8px 25px rgba(255, 69, 0, 0.3); text-transform: uppercase; letter-spacing: 1px;" onmouseover="this.style.transform='translateY(-2px)'; this.style.boxShadow='0 12px 35px rgba(255, 69, 0, 0.4), 0 12px 35px rgba(0, 170, 255, 0.3)';" onmouseout="this.style.transform='translateY(0)'; this.style.boxShadow='0 8px 25px rgba(255, 69, 0, 0.3)';">
                        Get Started
                    </a>
                <?php else: ?>
                    <div class="relative" @click.away="profileOpen = false">
                        <button @click="profileOpen = !profileOpen" style="display: flex; align-items: center; gap: 0.75rem; padding: 0.5rem 1rem; border-radius: 12px; transition: all 0.3s; background: rgba(255, 255, 255, 0.05); border: 1px solid var(--border);" onmouseover="this.style.background='rgba(255, 255, 255, 0.1)';" onmouseout="this.style.background='rgba(255, 255, 255, 0.05)';">
                            <div style="width: 40px; height: 40px; background: linear-gradient(135deg, var(--red), var(--blue)); border-radius: 50%; display: flex; align-items: center; justify-content: center; font-size: 1rem; font-weight: 800; color: white; box-shadow: 0 4px 15px rgba(255, 69, 0, 0.3);">
                                <?= strtoupper(substr($_SESSION['user_name'] ?? 'U', 0, 1)) ?>
                            </div>
                            <span style="font-size: 0.95rem; font-weight: 600; color: white;" class="hidden sm:block"><?= e($_SESSION['user_name'] ?? '') ?></span>
                            <svg style="width: 1rem; height: 1rem; color: var(--gray);" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M19 9l-7 7-7-7"/></svg>
                        </button>
                        <div x-show="profileOpen" x-cloak x-transition:enter="transition ease-out duration-100" x-transition:enter-start="opacity-0 scale-95" x-transition:enter-end="opacity-100 scale-100" x-transition:leave="transition ease-in duration-75" x-transition:leave-start="opacity-100 scale-100" x-transition:leave-end="opacity-0 scale-95"
                             class="absolute right-0 mt-2 w-56 bg-gray-800 border border-gray-700 rounded-xl shadow-2xl py-1 z-50">
                            <div class="px-4 py-3 border-b border-gray-700">
                                <p class="text-sm font-medium text-white"><?= e($_SESSION['user_name'] ?? '') ?></p>
                                <p class="text-xs text-gray-400 truncate"><?= e(current_user()['email'] ?? '') ?></p>
                            </div>
                            <a href="/dashboard" class="flex items-center gap-2 px-4 py-2.5 text-sm text-gray-300 hover:text-white hover:bg-gray-700/50 transition">
                                <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M4 6a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2H6a2 2 0 01-2-2V6zm10 0a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2h-2a2 2 0 01-2-2V6zM4 16a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2H6a2 2 0 01-2-2v-2zm10 0a2 2 0 012-2h2a2 2 0 012 2v2a2 2 0 01-2 2h-2a2 2 0 01-2-2v-2z"/></svg>
                                Dashboard
                            </a>
                            <a href="/profile" class="flex items-center gap-2 px-4 py-2.5 text-sm text-gray-300 hover:text-white hover:bg-gray-700/50 transition">
                                <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M16 7a4 4 0 11-8 0 4 4 0 018 0zM12 14a7 7 0 00-7 7h14a7 7 0 00-7-7z"/></svg>
                                Profile
                            </a>
                            <div class="border-t border-gray-700 my-1"></div>
                            <a href="/logout" class="flex items-center gap-2 px-4 py-2.5 text-sm text-red-400 hover:text-red-300 hover:bg-red-500/10 transition">
                                <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M17 16l4-4m0 0l-4-4m4 4H7m6 4v1a3 3 0 01-3 3H6a3 3 0 01-3-3V7a3 3 0 013-3h4a3 3 0 013 3v1"/></svg>
                                Sign Out
                            </a>
                        </div>
                    </div>
                <?php endif; ?>
                <button @click="mobileOpen = !mobileOpen" class="md:hidden p-2 text-gray-400 hover:text-white rounded-lg hover:bg-gray-800 transition">
                    <svg x-show="!mobileOpen" class="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M4 6h16M4 12h16M4 18h16"/></svg>
                    <svg x-show="mobileOpen" x-cloak class="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M6 18L18 6M6 6l12 12"/></svg>
                </button>
            </div>
        </div>
    </div>
    <!-- Mobile Menu -->
    <div x-show="mobileOpen" x-cloak x-transition class="md:hidden border-t border-gray-800/50">
        <div class="px-4 py-3 space-y-1">
            <a href="/" class="block px-3 py-2.5 text-sm font-medium rounded-lg text-gray-400 hover:text-white hover:bg-gray-800/50">Home</a>
            <a href="/plugins" class="block px-3 py-2.5 text-sm font-medium rounded-lg text-gray-400 hover:text-white hover:bg-gray-800/50">Plugins</a>
            <?php if (is_logged_in()): ?>
                <a href="/dashboard" class="block px-3 py-2.5 text-sm font-medium rounded-lg text-gray-400 hover:text-white hover:bg-gray-800/50">Dashboard</a>
                <?php if (is_admin()): ?>
                    <a href="/admin" class="block px-3 py-2.5 text-sm font-medium text-brand-400 rounded-lg hover:bg-brand-500/10">Admin Panel</a>
                <?php endif; ?>
            <?php else: ?>
                <a href="/login" class="block px-3 py-2.5 text-sm font-medium text-gray-400 hover:text-white rounded-lg hover:bg-gray-800/50">Sign In</a>
                <a href="/register" class="block px-3 py-2.5 text-sm font-medium text-brand-400 rounded-lg hover:bg-brand-500/10">Create Account</a>
            <?php endif; ?>
        </div>
    </div>
</nav>

<!-- Flash Messages -->
<?php if ($msg = get_flash('success')): ?>
<div x-data="{ show: true }" x-show="show" x-init="setTimeout(() => show = false, 5000)" x-transition:leave="transition ease-in duration-300" x-transition:leave-start="opacity-100" x-transition:leave-end="opacity-0" class="fixed top-20 right-4 z-50 max-w-sm">
    <div class="bg-emerald-500/10 border border-emerald-500/30 text-emerald-400 px-5 py-3 rounded-xl backdrop-blur-sm shadow-2xl flex items-center gap-3">
        <svg class="w-5 h-5 flex-shrink-0" fill="currentColor" viewBox="0 0 20 20"><path fill-rule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zm3.707-9.293a1 1 0 00-1.414-1.414L9 10.586 7.707 9.293a1 1 0 00-1.414 1.414l2 2a1 1 0 001.414 0l4-4z" clip-rule="evenodd"/></svg>
        <span class="text-sm font-medium"><?= e($msg) ?></span>
        <button @click="show = false" class="ml-auto text-emerald-400/60 hover:text-emerald-400">&times;</button>
    </div>
</div>
<?php endif; ?>
<?php if ($msg = get_flash('error')): ?>
<div x-data="{ show: true }" x-show="show" x-init="setTimeout(() => show = false, 6000)" x-transition:leave="transition ease-in duration-300" class="fixed top-20 right-4 z-50 max-w-sm">
    <div class="bg-red-500/10 border border-red-500/30 text-red-400 px-5 py-3 rounded-xl backdrop-blur-sm shadow-2xl flex items-center gap-3">
        <svg class="w-5 h-5 flex-shrink-0" fill="currentColor" viewBox="0 0 20 20"><path fill-rule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zM8.707 7.293a1 1 0 00-1.414 1.414L8.586 10l-1.293 1.293a1 1 0 101.414 1.414L10 11.414l1.293 1.293a1 1 0 001.414-1.414L11.414 10l1.293-1.293a1 1 0 00-1.414-1.414L10 8.586 8.707 7.293z" clip-rule="evenodd"/></svg>
        <span class="text-sm font-medium"><?= e($msg) ?></span>
        <button @click="show = false" class="ml-auto text-red-400/60 hover:text-red-400">&times;</button>
    </div>
</div>
<?php endif; ?>
<?php if ($msg = get_flash('warning')): ?>
<div x-data="{ show: true }" x-show="show" x-init="setTimeout(() => show = false, 6000)" x-transition:leave="transition ease-in duration-300" class="fixed top-20 right-4 z-50 max-w-sm">
    <div class="bg-amber-500/10 border border-amber-500/30 text-amber-400 px-5 py-3 rounded-xl backdrop-blur-sm shadow-2xl flex items-center gap-3">
        <svg class="w-5 h-5 flex-shrink-0" fill="currentColor" viewBox="0 0 20 20"><path fill-rule="evenodd" d="M8.257 3.099c.765-1.36 2.722-1.36 3.486 0l5.58 9.92c.75 1.334-.213 2.98-1.742 2.98H4.42c-1.53 0-2.493-1.646-1.743-2.98l5.58-9.92zM11 13a1 1 0 11-2 0 1 1 0 012 0zm-1-8a1 1 0 00-1 1v3a1 1 0 002 0V6a1 1 0 00-1-1z" clip-rule="evenodd"/></svg>
        <span class="text-sm font-medium"><?= e($msg) ?></span>
        <button @click="show = false" class="ml-auto text-amber-400/60 hover:text-amber-400">&times;</button>
    </div>
</div>
<?php endif; ?>

<main class="flex-1">
