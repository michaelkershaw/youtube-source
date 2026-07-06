<?php
$pageTitle = 'TellyMedia - Professional VirtualDJ Plugins';

// Get stats for homepage
$stats = [
    'total_plugins' => Database::count('plugins', 'is_active = 1'),
    'total_users' => Database::count('users'),
    'total_downloads' => Database::fetch("SELECT COALESCE(SUM(downloads_count), 0) as total FROM plugins")['total'],
];

// Get featured plugins
$featuredPlugins = Database::fetchAll(
    "SELECT p.*, 
            (SELECT MIN(price) FROM plugin_pricings WHERE plugin_id = p.id AND is_active = 1) as min_price,
            (SELECT image_path FROM plugin_images WHERE plugin_id = p.id AND is_featured = 1 LIMIT 1) as featured_image
     FROM plugins p 
     WHERE p.is_active = 1 AND p.is_featured = 1 
     ORDER BY p.sales_count DESC, p.downloads_count DESC 
     LIMIT 6"
);

require BASE_PATH . '/includes/header.php';
?>

<!-- Hero Section - VirtualDJ Style -->
<section class="vdj-hero">
    <!-- Background Video - Full Size -->
    <video class="vdj-hero-video" autoplay muted loop playsinline>
        <source src="<?= BASE_URL ?>/tvlaptop.mp4" type="video/mp4">
    </video>
    
    <!-- Overlay -->
    <div class="vdj-hero-overlay"></div>
    
    <!-- Particles -->
    <div class="particles-container">
        <?php for($i = 1; $i <= 20; $i++): ?>
        <div class="particle particle-<?= $i ?>"></div>
        <?php endfor; ?>
    </div>
    
    <!-- Hero Content -->
    <div class="vdj-hero-content">
        <div class="vdj-container">
            <!-- Main Headline - VirtualDJ Style -->
            <h1 class="vdj-headline">
                THE FUTURE OF DJING IS HERE NOW
            </h1>
            
            <!-- Logo - Centered -->
            <div class="vdj-logo-wrapper">
                <img src="<?= BASE_URL ?>/mainlogo.png" alt="TellyMedia" class="vdj-logo">
            </div>
            
            <!-- Sub Headline -->
            <h2 class="vdj-subheadline">
                <span class="vdj-highlight-red">PROFESSIONAL</span> VIRTUALDJ PLUGINS
            </h2>
            
            <!-- Description -->
            <p class="vdj-description">
                TellyMedia plugins use advanced technology and the power of modern computers to revolutionize what DJs can do. 
                With our plugins you can create stunning visual effects, beat-reactive shaders, and real-time video mixing that were simply not possible before.
            </p>
            
            <!-- CTA Buttons -->
            <div class="vdj-cta-group">
                <a href="/plugins" class="vdj-btn vdj-btn-primary">
                    <span>GET STARTED WITH TELLYMEDIA TODAY!</span>
                    <i class="fas fa-arrow-right"></i>
                </a>
                <a href="#features" class="vdj-btn vdj-btn-secondary">
                    <span>Learn More</span>
                </a>
            </div>
            
            <!-- Stats Bar -->
            <div class="vdj-stats-bar" data-aos="fade-up" data-aos-delay="400">
                <div class="vdj-stat">
                    <div class="vdj-stat-number counter" data-target="<?= $stats['total_downloads'] ?>">0</div>
                    <div class="vdj-stat-label">Downloads</div>
                </div>
                <div class="vdj-stat-divider"></div>
                <div class="vdj-stat">
                    <div class="vdj-stat-number counter" data-target="<?= $stats['total_plugins'] ?>">0</div>
                    <div class="vdj-stat-label">Plugins Available</div>
                </div>
                <div class="vdj-stat-divider"></div>
                <div class="vdj-stat">
                    <div class="vdj-stat-number">4.9★</div>
                    <div class="vdj-stat-label">User Rating</div>
                </div>
            </div>
        </div>
    </div>
    
    <!-- Scroll Down Indicator -->
    <a href="#features" class="scroll-indicator">
        <i class="fas fa-chevron-down"></i>
    </a>
</section>

<!-- Features Section - VirtualDJ Style -->
<section id="features" class="vdj-features">
    <div class="vdj-container">
        <!-- Section Header -->
        <div class="vdj-section-header" data-aos="fade-up">
            <h2 class="vdj-section-title">
                <span class="vdj-highlight-red">#1 MOST POPULAR</span> VIRTUALDJ PLUGIN MARKETPLACE
            </h2>
            <p class="vdj-section-subtitle">Powerful, yet easy to use</p>
        </div>
        
        <!-- Features Grid -->
        <div class="vdj-features-grid">
            <div class="vdj-feature-card" data-aos="fade-up" data-aos-delay="100">
                <div class="vdj-feature-icon">
                    <i class="fas fa-video"></i>
                </div>
                <h3 class="vdj-feature-title">Real-Time Video Effects</h3>
                <p class="vdj-feature-desc">
                    Create stunning visual effects with beat-reactive shaders and real-time video processing. 
                    Perfect for live performances and streaming.
                </p>
            </div>
            
            <div class="vdj-feature-card" data-aos="fade-up" data-aos-delay="200">
                <div class="vdj-feature-icon">
                    <i class="fas fa-sliders-h"></i>
                </div>
                <h3 class="vdj-feature-title">Advanced Controls</h3>
                <p class="vdj-feature-desc">
                    Full control over every parameter with MIDI mapping support. 
                    Customize your workflow to match your style.
                </p>
            </div>
            
            <div class="vdj-feature-card" data-aos="fade-up" data-aos-delay="300">
                <div class="vdj-feature-icon">
                    <i class="fas fa-bolt"></i>
                </div>
                <h3 class="vdj-feature-title">GPU Accelerated</h3>
                <p class="vdj-feature-desc">
                    Optimized for maximum performance using modern GPU technology. 
                    Smooth 60fps effects even on complex visuals.
                </p>
            </div>
            
            <div class="vdj-feature-card" data-aos="fade-up" data-aos-delay="400">
                <div class="vdj-feature-icon">
                    <i class="fas fa-puzzle-piece"></i>
                </div>
                <h3 class="vdj-feature-title">Easy Integration</h3>
                <p class="vdj-feature-desc">
                    Seamlessly integrates with VirtualDJ. 
                    Install and start using within minutes.
                </p>
            </div>
        </div>
    </div>
</section>

<!-- Why Choose TellyMedia Section -->
<section class="benefits-section">
    <div class="vdj-container">
        <div class="vdj-section-header" data-aos="fade-up">
            <h2 class="vdj-section-title">
                WHY CHOOSE <span class="vdj-highlight-red">TELLYMEDIA</span>
            </h2>
            <p class="vdj-section-subtitle">The professional choice for DJs worldwide</p>
        </div>
        
        <div class="benefits-grid">
            <div class="benefit-card" data-aos="zoom-in" data-aos-delay="100">
                <div class="benefit-icon">
                    <i class="fas fa-shield-alt"></i>
                </div>
                <h3 class="benefit-title">Secure Licensing</h3>
                <p class="benefit-desc">Advanced license management system with device tracking and instant activation.</p>
            </div>
            
            <div class="benefit-card" data-aos="zoom-in" data-aos-delay="200">
                <div class="benefit-icon">
                    <i class="fas fa-rocket"></i>
                </div>
                <h3 class="benefit-title">Instant Delivery</h3>
                <p class="benefit-desc">Download and activate your plugins immediately after purchase. No waiting.</p>
            </div>
            
            <div class="benefit-card" data-aos="zoom-in" data-aos-delay="300">
                <div class="benefit-icon">
                    <i class="fas fa-sync-alt"></i>
                </div>
                <h3 class="benefit-title">Auto Updates</h3>
                <p class="benefit-desc">Get the latest features and bug fixes automatically. Always stay current.</p>
            </div>
            
            <div class="benefit-card" data-aos="zoom-in" data-aos-delay="400">
                <div class="benefit-icon">
                    <i class="fas fa-headset"></i>
                </div>
                <h3 class="benefit-title">24/7 Support</h3>
                <p class="benefit-desc">Expert support team ready to help you with any questions or issues.</p>
            </div>
            
            <div class="benefit-card" data-aos="zoom-in" data-aos-delay="500">
                <div class="benefit-icon">
                    <i class="fas fa-users"></i>
                </div>
                <h3 class="benefit-title">Community</h3>
                <p class="benefit-desc">Join thousands of DJs sharing tips, tricks, and creative ideas.</p>
            </div>
            
            <div class="benefit-card" data-aos="zoom-in" data-aos-delay="600">
                <div class="benefit-icon">
                    <i class="fas fa-medal"></i>
                </div>
                <h3 class="benefit-title">Quality Guaranteed</h3>
                <p class="benefit-desc">All plugins are tested and verified to work perfectly with VirtualDJ.</p>
            </div>
        </div>
    </div>
</section>

<!-- VirtualDJ Styles -->
<style>
/* VirtualDJ Hero Section */
.vdj-hero {
    position: relative;
    min-height: 100vh;
    display: flex;
    align-items: center;
    justify-content: center;
    overflow: hidden;
    background: #000;
}

.vdj-hero-video {
    position: absolute;
    top: 50%;
    left: 50%;
    width: 100%;
    height: 100%;
    transform: translate(-50%, -50%);
    object-fit: contain;
    opacity: 0.2;
    z-index: 0;
}

.vdj-hero-overlay {
    position: absolute;
    inset: 0;
    background: linear-gradient(180deg, rgba(0,0,0,0.4) 0%, rgba(0,0,0,0.3) 80%, rgba(0,0,0,0.5) 100%);
    z-index: 1;
}

.vdj-hero-content {
    position: relative;
    z-index: 2;
    width: 100%;
    padding: 2rem 1.5rem;
}

.vdj-container {
    max-width: 1200px;
    margin: 0 auto;
    text-align: center;
}

/* Logo */
.vdj-logo-wrapper {
    margin: 2rem 0;
    display: flex;
    justify-content: center;
    align-items: center;
}

.vdj-logo {
    max-width: 250px;
    height: auto;
    filter: drop-shadow(0 0 30px rgba(255, 69, 0, 0.5)) drop-shadow(0 0 30px rgba(0, 170, 255, 0.5));
    animation: logoFloat 6s ease-in-out infinite;
}

@media (min-width: 768px) {
    .vdj-logo {
        max-width: 350px;
    }
}

/* Headlines - VirtualDJ Style */
.vdj-headline {
    font-size: 2.5rem;
    font-weight: 900;
    text-transform: uppercase;
    letter-spacing: 3px;
    color: white;
    margin-bottom: 1.5rem;
    line-height: 1.2;
    text-shadow: 0 4px 20px rgba(0, 0, 0, 0.8);
}

@media (min-width: 640px) {
    .vdj-headline {
        font-size: 3.5rem;
    }
}

@media (min-width: 1024px) {
    .vdj-headline {
        font-size: 4.5rem;
        letter-spacing: 5px;
    }
}

.vdj-subheadline {
    font-size: 1.5rem;
    font-weight: 700;
    text-transform: uppercase;
    letter-spacing: 2px;
    color: white;
    margin-bottom: 2rem;
}

@media (min-width: 640px) {
    .vdj-subheadline {
        font-size: 2rem;
    }
}

@media (min-width: 1024px) {
    .vdj-subheadline {
        font-size: 2.5rem;
        letter-spacing: 3px;
    }
}

.vdj-highlight-red {
    color: var(--red);
    text-shadow: 0 0 30px rgba(255, 69, 0, 0.8);
}

/* Description */
.vdj-description {
    font-size: 1rem;
    line-height: 1.8;
    color: #ccc;
    max-width: 800px;
    margin: 0 auto 3rem;
}

@media (min-width: 640px) {
    .vdj-description {
        font-size: 1.125rem;
    }
}

@media (min-width: 1024px) {
    .vdj-description {
        font-size: 1.25rem;
    }
}

/* CTA Buttons - VirtualDJ Style */
.vdj-cta-group {
    display: flex;
    flex-direction: column;
    gap: 1rem;
    align-items: center;
    margin-bottom: 4rem;
}

@media (min-width: 640px) {
    .vdj-cta-group {
        flex-direction: row;
        justify-content: center;
        gap: 1.5rem;
    }
}

.vdj-btn {
    display: inline-flex;
    align-items: center;
    gap: 0.75rem;
    padding: 1.25rem 2.5rem;
    font-size: 1rem;
    font-weight: 800;
    text-transform: uppercase;
    letter-spacing: 1.5px;
    text-decoration: none;
    border-radius: 4px;
    transition: all 0.3s;
    white-space: nowrap;
}

@media (min-width: 1024px) {
    .vdj-btn {
        padding: 1.5rem 3rem;
        font-size: 1.1rem;
    }
}

.vdj-btn-primary {
    background: linear-gradient(135deg, var(--red) 0%, #ff6b35 100%);
    color: white;
    box-shadow: 0 8px 25px rgba(255, 69, 0, 0.4);
}

.vdj-btn-primary:hover {
    transform: translateY(-3px);
    box-shadow: 0 12px 35px rgba(255, 69, 0, 0.6);
}

.vdj-btn-secondary {
    background: transparent;
    color: white;
    border: 2px solid white;
}

.vdj-btn-secondary:hover {
    background: white;
    color: #000;
}

/* Stats Bar - VirtualDJ Style */
.vdj-stats-bar {
    display: flex;
    flex-direction: column;
    gap: 2rem;
    align-items: center;
    padding: 2rem;
    background: rgba(255, 255, 255, 0.05);
    border-radius: 8px;
    backdrop-filter: blur(10px);
}

@media (min-width: 640px) {
    .vdj-stats-bar {
        flex-direction: row;
        justify-content: center;
        gap: 3rem;
    }
}

.vdj-stat {
    text-align: center;
}

.vdj-stat-number {
    font-size: 2.5rem;
    font-weight: 900;
    color: white;
    margin-bottom: 0.5rem;
    text-shadow: 0 0 20px rgba(255, 255, 255, 0.5);
}

@media (min-width: 1024px) {
    .vdj-stat-number {
        font-size: 3rem;
    }
}

.vdj-stat-label {
    font-size: 0.875rem;
    color: #999;
    text-transform: uppercase;
    letter-spacing: 1px;
}

.vdj-stat-divider {
    width: 1px;
    height: 50px;
    background: rgba(255, 255, 255, 0.2);
    display: none;
}

@media (min-width: 640px) {
    .vdj-stat-divider {
        display: block;
    }
}

/* Features Section - VirtualDJ Style */
.vdj-features {
    padding: 6rem 1.5rem;
    background: #0a0a0a;
}

.vdj-section-header {
    text-align: center;
    margin-bottom: 4rem;
}

.vdj-section-title {
    font-size: 2rem;
    font-weight: 900;
    text-transform: uppercase;
    letter-spacing: 2px;
    color: white;
    margin-bottom: 1rem;
}

@media (min-width: 640px) {
    .vdj-section-title {
        font-size: 2.5rem;
    }
}

@media (min-width: 1024px) {
    .vdj-section-title {
        font-size: 3rem;
        letter-spacing: 3px;
    }
}

.vdj-section-subtitle {
    font-size: 1.25rem;
    color: #999;
}

/* Features Grid */
.vdj-features-grid {
    display: grid;
    grid-template-columns: 1fr;
    gap: 2rem;
    max-width: 1200px;
    margin: 0 auto;
}

@media (min-width: 640px) {
    .vdj-features-grid {
        grid-template-columns: repeat(2, 1fr);
    }
}

@media (min-width: 1024px) {
    .vdj-features-grid {
        grid-template-columns: repeat(4, 1fr);
        gap: 2.5rem;
    }
}

.vdj-feature-card {
    padding: 2.5rem 2rem;
    background: linear-gradient(145deg, rgba(26, 26, 26, 0.8), rgba(10, 10, 10, 0.8));
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 8px;
    text-align: center;
    transition: all 0.3s;
}

.vdj-feature-card:hover {
    transform: translateY(-8px);
    border-color: var(--red);
    box-shadow: 0 15px 40px rgba(255, 69, 0, 0.3);
}

.vdj-feature-icon {
    width: 80px;
    height: 80px;
    margin: 0 auto 1.5rem;
    display: flex;
    align-items: center;
    justify-content: center;
    font-size: 2.5rem;
    color: var(--red);
    background: rgba(255, 69, 0, 0.1);
    border-radius: 50%;
    transition: all 0.3s;
}

.vdj-feature-card:hover .vdj-feature-icon {
    background: rgba(255, 69, 0, 0.2);
    transform: scale(1.1);
}

.vdj-feature-title {
    font-size: 1.25rem;
    font-weight: 700;
    color: white;
    margin-bottom: 1rem;
    text-transform: uppercase;
    letter-spacing: 1px;
}

.vdj-feature-desc {
    font-size: 0.95rem;
    line-height: 1.7;
    color: #999;
}

/* Scroll Indicator */
.scroll-indicator {
    position: absolute;
    bottom: 2rem;
    left: 50%;
    transform: translateX(-50%);
    width: 40px;
    height: 40px;
    display: flex;
    align-items: center;
    justify-content: center;
    color: white;
    font-size: 1.25rem;
    animation: bounce 2s infinite;
    z-index: 10;
    opacity: 0.7;
    transition: opacity 0.3s;
}

.scroll-indicator:hover {
    opacity: 1;
}

@keyframes bounce {
    0%, 100% { transform: translateX(-50%) translateY(0); }
    50% { transform: translateX(-50%) translateY(10px); }
}

/* Benefits Section */
.benefits-section {
    padding: 6rem 1.5rem;
    background: linear-gradient(180deg, #0a0a0a 0%, #050505 100%);
}

.benefits-grid {
    display: grid;
    grid-template-columns: 1fr;
    gap: 2rem;
    max-width: 1200px;
    margin: 0 auto;
}

@media (min-width: 640px) {
    .benefits-grid {
        grid-template-columns: repeat(2, 1fr);
    }
}

@media (min-width: 1024px) {
    .benefits-grid {
        grid-template-columns: repeat(3, 1fr);
        gap: 2.5rem;
    }
}

.benefit-card {
    padding: 3rem 2rem;
    background: linear-gradient(145deg, rgba(26, 26, 26, 0.6), rgba(10, 10, 10, 0.6));
    border: 1px solid rgba(255, 255, 255, 0.05);
    border-radius: 1rem;
    text-align: center;
    transition: all 0.4s cubic-bezier(0.4, 0, 0.2, 1);
    backdrop-filter: blur(10px);
}

.benefit-card:hover {
    transform: translateY(-10px) scale(1.02);
    border-color: var(--red);
    background: linear-gradient(145deg, rgba(26, 26, 26, 0.8), rgba(10, 10, 10, 0.8));
    box-shadow: 0 20px 50px rgba(255, 69, 0, 0.3);
}

.benefit-icon {
    width: 80px;
    height: 80px;
    margin: 0 auto 1.5rem;
    display: flex;
    align-items: center;
    justify-content: center;
    font-size: 2.5rem;
    background: linear-gradient(135deg, rgba(255, 69, 0, 0.2), rgba(0, 170, 255, 0.2));
    border-radius: 50%;
    transition: all 0.4s;
}

.benefit-card:hover .benefit-icon {
    transform: rotateY(360deg) scale(1.1);
    background: linear-gradient(135deg, rgba(255, 69, 0, 0.3), rgba(0, 170, 255, 0.3));
}

.benefit-icon i {
    background: linear-gradient(135deg, var(--red), var(--blue));
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
}

.benefit-title {
    font-size: 1.25rem;
    font-weight: 700;
    color: white;
    margin-bottom: 1rem;
    text-transform: uppercase;
    letter-spacing: 1px;
}

.benefit-desc {
    font-size: 0.95rem;
    line-height: 1.7;
    color: #999;
}

@media (min-width: 768px) {
    .hero-grid {
        grid-template-columns: 1fr 1fr;
        gap: 4rem;
        padding: 5rem 0;
    }
}

@media (min-width: 1024px) {
    .hero-grid {
        grid-template-columns: 1.2fr 0.8fr;
        gap: 6rem;
        padding: 6rem 0;
    }
}

/* Logo */
.hero-logo-img {
    max-width: 200px;
    height: auto;
    filter: drop-shadow(0 0 30px rgba(255, 69, 0, 0.5)) drop-shadow(0 0 30px rgba(0, 170, 255, 0.5));
    animation: logoFloat 6s ease-in-out infinite;
}

@media (min-width: 768px) {
    .hero-logo-img {
        max-width: 280px;
    }
}

/* Title */
.hero-title-modern {
    font-size: 2.5rem;
    font-weight: 900;
    line-height: 1.1;
    margin-bottom: 1.5rem;
    text-transform: uppercase;
    letter-spacing: 2px;
}

@media (min-width: 640px) {
    .hero-title-modern {
        font-size: 3rem;
    }
}

@media (min-width: 1024px) {
    .hero-title-modern {
        font-size: 4rem;
        letter-spacing: 3px;
    }
}

.hero-title-line {
    display: block;
    color: white;
    text-shadow: 0 0 30px rgba(255, 255, 255, 0.3);
}

.hero-title-highlight {
    display: block;
    margin: 0.5rem 0;
}

.text-red {
    color: var(--red);
    text-shadow: 0 0 40px rgba(255, 69, 0, 1), 0 0 80px rgba(255, 69, 0, 0.5);
}

.text-blue {
    color: var(--blue);
    text-shadow: 0 0 40px rgba(0, 170, 255, 1), 0 0 80px rgba(0, 170, 255, 0.5);
}

/* Subtitle */
.hero-subtitle-modern {
    font-size: 1rem;
    color: #b0b0b0;
    margin-bottom: 2.5rem;
    line-height: 1.7;
    font-weight: 400;
}

@media (min-width: 640px) {
    .hero-subtitle-modern {
        font-size: 1.15rem;
    }
}

@media (min-width: 1024px) {
    .hero-subtitle-modern {
        font-size: 1.35rem;
        max-width: 600px;
        margin-bottom: 3.5rem;
    }
}

/* CTA Buttons */
.hero-cta-modern {
    display: flex;
    flex-direction: column;
    gap: 1rem;
    margin-bottom: 3rem;
}

@media (min-width: 640px) {
    .hero-cta-modern {
        flex-direction: row;
        gap: 1.5rem;
        margin-bottom: 4rem;
    }
}

.btn-primary-modern {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    gap: 0.75rem;
    padding: 1.25rem 2rem;
    background: linear-gradient(90deg, var(--red) 0%, var(--blue) 100%);
    color: white;
    text-decoration: none;
    border-radius: 50px;
    font-weight: 800;
    font-size: 1rem;
    text-transform: uppercase;
    letter-spacing: 1.5px;
    transition: all 0.4s;
    box-shadow: 0 10px 30px rgba(255, 69, 0, 0.4), 0 10px 30px rgba(0, 170, 255, 0.2);
    white-space: nowrap;
}

@media (min-width: 1024px) {
    .btn-primary-modern {
        padding: 1.5rem 3rem;
        font-size: 1.1rem;
        letter-spacing: 2px;
    }
}

.btn-primary-modern:hover {
    transform: translateY(-3px);
    box-shadow: 0 15px 40px rgba(255, 69, 0, 0.6), 0 15px 40px rgba(0, 170, 255, 0.4);
}

.btn-secondary-modern {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    gap: 0.75rem;
    padding: 1.25rem 2rem;
    background: transparent;
    color: white;
    text-decoration: none;
    border: 2px solid var(--blue);
    border-radius: 50px;
    font-weight: 700;
    font-size: 1rem;
    text-transform: uppercase;
    letter-spacing: 1.5px;
    transition: all 0.4s;
    white-space: nowrap;
}

@media (min-width: 1024px) {
    .btn-secondary-modern {
        padding: 1.5rem 3rem;
        font-size: 1.1rem;
        letter-spacing: 2px;
        border-width: 3px;
    }
}

.btn-secondary-modern:hover {
    background: var(--blue);
    transform: translateY(-3px);
    box-shadow: 0 15px 40px rgba(0, 170, 255, 0.5);
}

/* Stats Grid */
.stats-grid-modern {
    display: grid;
    grid-template-columns: 1fr;
    gap: 1rem;
}

@media (min-width: 640px) {
    .stats-grid-modern {
        grid-template-columns: repeat(3, 1fr);
        gap: 1.5rem;
    }
}

.stat-card-modern {
    display: flex;
    align-items: center;
    gap: 1rem;
    padding: 1.5rem;
    background: linear-gradient(145deg, rgba(26, 26, 26, 0.8), rgba(10, 10, 10, 0.8));
    border-radius: 1rem;
    border: 1px solid var(--border);
    backdrop-filter: blur(10px);
    transition: all 0.3s;
}

.stat-card-modern:hover {
    transform: translateY(-5px);
    box-shadow: 0 10px 30px rgba(0, 0, 0, 0.5);
}

.stat-card-modern.stat-red {
    border-left: 3px solid var(--red);
}

.stat-card-modern.stat-blue {
    border-left: 3px solid var(--blue);
}

.stat-card-modern.stat-gold {
    border-left: 3px solid #ffd700;
}

.stat-icon {
    font-size: 2rem;
    width: 50px;
    height: 50px;
    display: flex;
    align-items: center;
    justify-content: center;
    border-radius: 12px;
    background: rgba(255, 255, 255, 0.05);
}

.stat-red .stat-icon {
    color: var(--red);
    background: rgba(255, 69, 0, 0.1);
}

.stat-blue .stat-icon {
    color: var(--blue);
    background: rgba(0, 170, 255, 0.1);
}

.stat-gold .stat-icon {
    color: #ffd700;
    background: rgba(255, 215, 0, 0.1);
}

.stat-content {
    flex: 1;
}

.stat-number {
    font-size: 1.75rem;
    font-weight: 900;
    color: white;
    line-height: 1;
    margin-bottom: 0.25rem;
}

@media (min-width: 1024px) {
    .stat-number {
        font-size: 2.25rem;
    }
}

.stat-label {
    font-size: 0.75rem;
    color: var(--gray);
    text-transform: uppercase;
    letter-spacing: 1px;
    font-weight: 600;
}

/* Visual Container */
.hero-visual-modern {
    display: none;
}

@media (min-width: 768px) {
    .hero-visual-modern {
        display: flex;
        align-items: center;
        justify-content: center;
    }
}

.visual-container {
    position: relative;
    width: 100%;
    max-width: 400px;
    aspect-ratio: 1;
}

@media (min-width: 1024px) {
    .visual-container {
        max-width: 450px;
    }
}

/* Rings */
.ring {
    position: absolute;
    border-radius: 50%;
    border: 2px solid;
}

.ring-outer {
    inset: -10%;
    border-color: rgba(255, 69, 0, 0.3);
    animation: rotate 20s linear infinite;
}

.ring-middle {
    inset: -5%;
    border-color: rgba(0, 170, 255, 0.3);
    animation: rotate 15s linear infinite reverse;
}

/* Glow */
.glow-effect {
    position: absolute;
    inset: 0;
    background: radial-gradient(circle, rgba(255, 69, 0, 0.3) 0%, rgba(0, 170, 255, 0.3) 50%, transparent 70%);
    border-radius: 50%;
    animation: pulse 4s ease-in-out infinite;
    filter: blur(40px);
}

/* Central Badge */
.central-badge {
    position: absolute;
    inset: 10%;
    background: linear-gradient(145deg, rgba(26, 26, 26, 0.95), rgba(10, 10, 10, 0.95));
    border-radius: 50%;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    border: 3px solid var(--border);
    box-shadow: 0 30px 80px rgba(0, 0, 0, 0.8), 
                inset 0 0 50px rgba(255, 69, 0, 0.1), 
                inset 0 0 50px rgba(0, 170, 255, 0.1);
    backdrop-filter: blur(10px);
}

.badge-icon {
    font-size: 5rem;
    margin-bottom: 1rem;
    filter: drop-shadow(0 0 30px rgba(255, 69, 0, 0.8)) drop-shadow(0 0 30px rgba(0, 170, 255, 0.8));
    animation: float 6s ease-in-out infinite;
}

@media (min-width: 1024px) {
    .badge-icon {
        font-size: 6rem;
    }
}

.badge-title {
    font-size: 1.5rem;
    font-weight: 900;
    letter-spacing: 2px;
    line-height: 1;
    margin-bottom: 0.5rem;
}

@media (min-width: 1024px) {
    .badge-title {
        font-size: 2rem;
    }
}

.badge-subtitle {
    font-size: 0.65rem;
    color: var(--gray);
    text-transform: uppercase;
    letter-spacing: 2px;
    font-weight: 600;
}

@media (min-width: 1024px) {
    .badge-subtitle {
        font-size: 0.75rem;
        letter-spacing: 3px;
    }
}

/* Floating Icons */
.float-icon {
    position: absolute;
    font-size: 1.5rem;
    filter: drop-shadow(0 0 15px rgba(255, 69, 0, 0.6));
}

@media (min-width: 1024px) {
    .float-icon {
        font-size: 2rem;
    }
}

.float-1 {
    top: 5%;
    left: -5%;
    animation: float 8s ease-in-out infinite;
}

.float-2 {
    top: 15%;
    right: -5%;
    animation: float 10s ease-in-out infinite 1s;
    filter: drop-shadow(0 0 15px rgba(0, 170, 255, 0.6));
}

.float-3 {
    bottom: 10%;
    left: 0%;
    animation: float 9s ease-in-out infinite 2s;
}

.float-4 {
    bottom: 20%;
    right: 5%;
    animation: float 11s ease-in-out infinite 3s;
    filter: drop-shadow(0 0 15px rgba(0, 170, 255, 0.6));
}

/* Animations */
@keyframes gradientShift {
    0%, 100% { opacity: 1; }
    50% { opacity: 0.7; }
}

@keyframes rotate {
    from { transform: rotate(0deg); }
    to { transform: rotate(360deg); }
}

@keyframes pulse {
    0%, 100% { transform: scale(1); opacity: 0.6; }
    50% { transform: scale(1.1); opacity: 1; }
}

@keyframes float {
    0%, 100% { transform: translateY(0px); }
    50% { transform: translateY(-20px); }
}

@keyframes logoFloat {
    0%, 100% { transform: translateY(0); }
    50% { transform: translateY(-15px); }
}

/* Testimonials Section */
.testimonials-section {
    padding: 6rem 1.5rem;
    background: linear-gradient(180deg, #050505 0%, #0a0a0a 100%);
}

.testimonials-grid {
    display: grid;
    grid-template-columns: 1fr;
    gap: 2rem;
    max-width: 1200px;
    margin: 0 auto;
}

@media (min-width: 768px) {
    .testimonials-grid {
        grid-template-columns: repeat(3, 1fr);
        gap: 2.5rem;
    }
}

.testimonial-card {
    padding: 2.5rem 2rem;
    background: linear-gradient(145deg, rgba(26, 26, 26, 0.8), rgba(10, 10, 10, 0.8));
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 1rem;
    transition: all 0.4s;
    backdrop-filter: blur(10px);
}

.testimonial-card:hover {
    transform: translateY(-8px);
    border-color: var(--blue);
    box-shadow: 0 20px 50px rgba(0, 170, 255, 0.2);
}

.testimonial-stars {
    display: flex;
    gap: 0.25rem;
    margin-bottom: 1.5rem;
    color: #ffd700;
    font-size: 1rem;
}

.testimonial-text {
    font-size: 1rem;
    line-height: 1.8;
    color: #ccc;
    margin-bottom: 2rem;
    font-style: italic;
}

.testimonial-author {
    display: flex;
    align-items: center;
    gap: 1rem;
}

.author-avatar {
    width: 50px;
    height: 50px;
    background: linear-gradient(135deg, var(--red), var(--blue));
    border-radius: 50%;
    display: flex;
    align-items: center;
    justify-content: center;
    font-weight: 800;
    color: white;
    font-size: 1.25rem;
}

.author-name {
    font-weight: 700;
    color: white;
    margin-bottom: 0.25rem;
}

.author-role {
    font-size: 0.875rem;
    color: var(--gray);
}

/* Newsletter Section */
.newsletter-section {
    padding: 6rem 1.5rem;
    background: linear-gradient(180deg, var(--darker) 0%, #0a0a0a 100%);
}

.newsletter-content {
    max-width: 800px;
    margin: 0 auto;
    text-align: center;
    padding: 4rem 2rem;
    background: linear-gradient(145deg, rgba(26, 26, 26, 0.8), rgba(10, 10, 10, 0.8));
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 1.5rem;
    backdrop-filter: blur(10px);
}

.newsletter-icon {
    width: 80px;
    height: 80px;
    margin: 0 auto 2rem;
    display: flex;
    align-items: center;
    justify-content: center;
    font-size: 2.5rem;
    background: linear-gradient(135deg, var(--red), var(--blue));
    border-radius: 50%;
    color: white;
    animation: pulse 2s ease-in-out infinite;
}

.newsletter-title {
    font-size: 2rem;
    font-weight: 900;
    text-transform: uppercase;
    letter-spacing: 2px;
    color: white;
    margin-bottom: 1rem;
}

@media (min-width: 768px) {
    .newsletter-title {
        font-size: 2.5rem;
    }
}

.newsletter-subtitle {
    font-size: 1.125rem;
    color: #ccc;
    margin-bottom: 2.5rem;
    line-height: 1.7;
}

.newsletter-form {
    display: flex;
    flex-direction: column;
    gap: 1rem;
    max-width: 600px;
    margin: 0 auto 1.5rem;
}

@media (min-width: 640px) {
    .newsletter-form {
        flex-direction: row;
    }
}

.newsletter-input {
    flex: 1;
    padding: 1.25rem 1.5rem;
    background: rgba(255, 255, 255, 0.05);
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 0.5rem;
    color: white;
    font-size: 1rem;
    transition: all 0.3s;
}

.newsletter-input:focus {
    outline: none;
    border-color: var(--blue);
    background: rgba(255, 255, 255, 0.08);
    box-shadow: 0 0 0 3px rgba(0, 170, 255, 0.1);
}

.newsletter-input::placeholder {
    color: #666;
}

.newsletter-btn {
    padding: 1.25rem 2.5rem;
    background: linear-gradient(90deg, var(--red) 0%, var(--blue) 100%);
    color: white;
    border: none;
    border-radius: 0.5rem;
    font-weight: 800;
    font-size: 1rem;
    text-transform: uppercase;
    letter-spacing: 1px;
    cursor: pointer;
    transition: all 0.3s;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 0.5rem;
    white-space: nowrap;
}

.newsletter-btn:hover {
    transform: translateY(-2px);
    box-shadow: 0 10px 30px rgba(255, 69, 0, 0.4), 0 10px 30px rgba(0, 170, 255, 0.3);
}

.newsletter-privacy {
    font-size: 0.875rem;
    color: #666;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 0.5rem;
}

/* CTA Section */
.cta-section {
    padding: 6rem 1.5rem;
    background: var(--darker);
}

.cta-card {
    position: relative;
    max-width: 1000px;
    margin: 0 auto;
    padding: 4rem 2rem;
    background: linear-gradient(135deg, var(--red) 0%, var(--blue) 100%);
    border-radius: 1.5rem;
    overflow: hidden;
    box-shadow: 0 30px 80px rgba(255, 69, 0, 0.3);
}

.cta-pattern {
    position: absolute;
    inset: 0;
    background-image: 
        repeating-linear-gradient(45deg, transparent, transparent 35px, rgba(255,255,255,.05) 35px, rgba(255,255,255,.05) 70px);
    opacity: 0.3;
}

.cta-content {
    position: relative;
    z-index: 2;
    text-align: center;
}

.cta-title {
    font-size: 2.5rem;
    font-weight: 900;
    color: white;
    margin-bottom: 1rem;
    text-shadow: 0 4px 20px rgba(0, 0, 0, 0.3);
}

@media (min-width: 768px) {
    .cta-title {
        font-size: 3rem;
    }
}

.cta-subtitle {
    font-size: 1.125rem;
    color: rgba(255, 255, 255, 0.9);
    margin-bottom: 2.5rem;
    max-width: 700px;
    margin-left: auto;
    margin-right: auto;
    line-height: 1.7;
}

.cta-buttons {
    display: flex;
    flex-direction: column;
    gap: 1rem;
    align-items: center;
    justify-content: center;
}

@media (min-width: 640px) {
    .cta-buttons {
        flex-direction: row;
        gap: 1.5rem;
    }
}

.cta-buttons .vdj-btn-primary {
    background: white;
    color: var(--red);
    box-shadow: 0 10px 30px rgba(0, 0, 0, 0.3);
}

.cta-buttons .vdj-btn-primary:hover {
    transform: translateY(-3px);
    box-shadow: 0 15px 40px rgba(0, 0, 0, 0.4);
}

.cta-buttons .vdj-btn-secondary {
    background: rgba(255, 255, 255, 0.1);
    border-color: white;
    color: white;
}

.cta-buttons .vdj-btn-secondary:hover {
    background: rgba(255, 255, 255, 0.2);
}
</style>

<script>
// Counter Animation
document.addEventListener('DOMContentLoaded', function() {
    const counters = document.querySelectorAll('.counter');
    const speed = 200;
    
    const animateCounter = (counter) => {
        const target = +counter.getAttribute('data-target');
        const increment = target / speed;
        
        const updateCount = () => {
            const count = +counter.innerText.replace(/[^0-9]/g, '');
            
            if (count < target) {
                counter.innerText = Math.ceil(count + increment).toLocaleString() + '+';
                setTimeout(updateCount, 1);
            } else {
                counter.innerText = target.toLocaleString() + '+';
            }
        };
        
        updateCount();
    };
    
    // Intersection Observer for counter animation
    const observer = new IntersectionObserver((entries) => {
        entries.forEach(entry => {
            if (entry.isIntersecting) {
                animateCounter(entry.target);
                observer.unobserve(entry.target);
            }
        });
    }, { threshold: 0.5 });
    
    counters.forEach(counter => {
        observer.observe(counter);
    });
});
</script>

<!-- Testimonials Section -->
<section class="testimonials-section">
    <div class="vdj-container">
        <div class="vdj-section-header" data-aos="fade-up">
            <h2 class="vdj-section-title">
                TRUSTED BY <span class="vdj-highlight-red">PROFESSIONAL DJS</span>
            </h2>
            <p class="vdj-section-subtitle">See what our users are saying</p>
        </div>
        
        <div class="testimonials-grid">
            <div class="testimonial-card" data-aos="fade-right" data-aos-delay="100">
                <div class="testimonial-stars">
                    <i class="fas fa-star"></i>
                    <i class="fas fa-star"></i>
                    <i class="fas fa-star"></i>
                    <i class="fas fa-star"></i>
                    <i class="fas fa-star"></i>
                </div>
                <p class="testimonial-text">
                    "TellyMedia plugins have completely transformed my live performances. The visual effects are stunning and the performance is flawless!"
                </p>
                <div class="testimonial-author">
                    <div class="author-avatar">DJ</div>
                    <div>
                        <div class="author-name">DJ Mike K</div>
                        <div class="author-role">Professional DJ</div>
                    </div>
                </div>
            </div>
            
            <div class="testimonial-card" data-aos="fade-up" data-aos-delay="200">
                <div class="testimonial-stars">
                    <i class="fas fa-star"></i>
                    <i class="fas fa-star"></i>
                    <i class="fas fa-star"></i>
                    <i class="fas fa-star"></i>
                    <i class="fas fa-star"></i>
                </div>
                <p class="testimonial-text">
                    "The licensing system is so easy to use and the instant delivery means I can start using plugins right away. Highly recommend!"
                </p>
                <div class="testimonial-author">
                    <div class="author-avatar">DJ</div>
                    <div>
                        <div class="author-name">Sarah Chen</div>
                        <div class="author-role">Club DJ</div>
                    </div>
                </div>
            </div>
            
            <div class="testimonial-card" data-aos="fade-left" data-aos-delay="300">
                <div class="testimonial-stars">
                    <i class="fas fa-star"></i>
                    <i class="fas fa-star"></i>
                    <i class="fas fa-star"></i>
                    <i class="fas fa-star"></i>
                    <i class="fas fa-star"></i>
                </div>
                <p class="testimonial-text">
                    "Best VirtualDJ plugins I've ever used. The quality is outstanding and the support team is incredibly helpful!"
                </p>
                <div class="testimonial-author">
                    <div class="author-avatar">DJ</div>
                    <div>
                        <div class="author-name">Alex Rodriguez</div>
                        <div class="author-role">Mobile DJ</div>
                    </div>
                </div>
            </div>
        </div>
    </div>
</section>

<!-- Featured Plugins -->
<section class="featured-plugins" style="padding: 6rem 0; background: var(--darker);">
    <div class="container">
        <h2 class="section-title-center" data-aos="fade-up">
            <span class="title-telly">FEATURED</span> <span class="title-media">PLUGINS</span>
        </h2>
        <p class="section-subtitle" data-aos="fade-up" data-aos-delay="100">Discover our most popular VirtualDJ plugins</p>
        
        <?php if (!empty($featuredPlugins)): ?>
        <div class="plugins-grid" style="display: grid; grid-template-columns: repeat(auto-fit, minmax(320px, 1fr)); gap: 2.5rem; margin-bottom: 4rem;">
            <?php $delay = 0; foreach ($featuredPlugins as $plugin): $delay += 100; ?>
            <div class="plugin-card" data-aos="fade-up" data-aos-delay="<?= $delay ?>" style="background: linear-gradient(145deg, var(--light), #1f1f1f); border: 1px solid var(--border); border-radius: 1.25rem; overflow: hidden; transition: all 0.4s cubic-bezier(0.4, 0, 0.2, 1); position: relative;">
                <!-- Glow effect on hover -->
                <div style="position: absolute; inset: 0; background: linear-gradient(45deg, rgba(255, 69, 0, 0.1), rgba(0, 170, 255, 0.1)); opacity: 0; transition: opacity 0.4s; border-radius: 1.25rem;"></div>
                
                <div class="plugin-image" style="position: relative; height: 220px; background: linear-gradient(135deg, var(--dark), #1a1a1a); display: flex; align-items: center; justify-content: center; overflow: hidden;">
                    <?php if ($plugin['featured_image']): ?>
                        <img src="<?= BASE_URL ?>/uploads/images/<?= e($plugin['featured_image']) ?>" alt="<?= e($plugin['name']) ?>" style="width: 100%; height: 100%; object-fit: cover; transition: transform 0.4s;">
                    <?php else: ?>
                        <div style="text-align: center; background: linear-gradient(135deg, rgba(255, 69, 0, 0.2), rgba(0, 170, 255, 0.2)); width: 100%; height: 100%; display: flex; align-items: center; justify-content: center;">
                            <span style="font-size: 4rem; filter: drop-shadow(0 0 20px rgba(255, 69, 0, 0.5));">🎵</span>
                        </div>
                    <?php endif; ?>
                    
                    <?php if ($plugin['is_featured']): ?>
                        <span style="position: absolute; top: 1rem; right: 1rem; background: linear-gradient(135deg, var(--red), #ff6b35); color: white; padding: 0.5rem 1rem; border-radius: 2rem; font-size: 0.75rem; font-weight: 800; letter-spacing: 1px; text-transform: uppercase; box-shadow: 0 4px 15px rgba(255, 69, 0, 0.4); z-index: 10;">
                            ⭐ FEATURED
                        </span>
                    <?php endif; ?>
                </div>
                
                <div style="padding: 2rem; position: relative; z-index: 2;">
                    <h3 style="font-size: 1.5rem; font-weight: 800; color: white; margin-bottom: 0.75rem; line-height: 1.2; background: linear-gradient(135deg, white, #e0e0e0); -webkit-background-clip: text; -webkit-text-fill-color: transparent; background-clip: text;">
                        <?= e($plugin['name']) ?>
                    </h3>
                    
                    <p style="color: var(--blue); font-size: 0.875rem; margin-bottom: 1rem; font-weight: 600; text-transform: uppercase; letter-spacing: 1px;">
                        <?= e($plugin['category'] ?? 'General') ?>
                    </p>
                    
                    <p style="color: var(--text); margin-bottom: 1.5rem; line-height: 1.7; font-size: 0.95rem;">
                        <?= e(substr((string)($plugin['description'] ?? ''), 0, 120)) ?>...
                    </p>
                    
                    <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 1.5rem; padding: 1rem; background: rgba(0, 0, 0, 0.3); border-radius: 0.75rem;">
                        <div style="display: flex; align-items: center; gap: 0.5rem;">
                            <i class="fas fa-download" style="color: var(--blue); font-size: 0.875rem;"></i>
                            <span style="color: var(--gray); font-size: 0.875rem; font-weight: 600;">
                                <?= number_format($plugin['downloads_count']) ?>
                            </span>
                        </div>
                        <div style="display: flex; align-items: center; gap: 0.5rem;">
                            <i class="fas fa-star" style="color: #ffd700; font-size: 0.875rem;"></i>
                            <span style="color: var(--gray); font-size: 0.875rem; font-weight: 600;">
                                <?= isset($plugin['rating']) ? $plugin['rating'] : '4.9' ?>
                            </span>
                        </div>
                    </div>
                    
                    <div style="margin-bottom: 1.5rem;">
                        <?php if ($plugin['min_price'] > 0): ?>
                            <span style="color: var(--blue); font-weight: 800; font-size: 1.25rem; text-shadow: 0 0 10px rgba(0, 170, 255, 0.5);">
                                From <?= format_price($plugin['min_price']) ?>
                            </span>
                        <?php else: ?>
                            <span style="color: #10b981; font-weight: 800; font-size: 1.25rem; text-shadow: 0 0 10px rgba(16, 185, 129, 0.5);">
                                FREE
                            </span>
                        <?php endif; ?>
                    </div>
                    
                    <a href="/plugin/<?= e($plugin['slug']) ?>" class="btn-hero" style="display: block; text-align: center; padding: 1rem 2rem; font-size: 0.95rem; font-weight: 800; letter-spacing: 1px; text-transform: uppercase; text-decoration: none; border-radius: 0.75rem; transition: all 0.3s; box-shadow: 0 8px 25px rgba(255, 69, 0, 0.3);">
                        VIEW DETAILS
                    </a>
                </div>
            </div>
            <?php endforeach; ?>
        </div>
        <?php else: ?>
        <div style="text-align: center; padding: 4rem; background: var(--light); border-radius: 1.25rem; border: 1px solid var(--border);">
            <div style="font-size: 3rem; margin-bottom: 1rem;">🎵</div>
            <h3 style="color: white; font-size: 1.5rem; font-weight: 700; margin-bottom: 1rem;">No Featured Plugins Yet</h3>
            <p style="color: var(--gray); margin-bottom: 2rem;">Check back soon for our featured VirtualDJ plugins!</p>
            <a href="/plugins" class="btn-secondary">Browse All Plugins</a>
        </div>
        <?php endif; ?>
        
        <?php if (!empty($featuredPlugins)): ?>
        <div style="text-align: center;">
            <a href="/plugins" class="btn-secondary" style="padding: 1.25rem 3rem; font-size: 1rem;">
                VIEW ALL PLUGINS
            </a>
        </div>
        <?php endif; ?>
    </div>
</section>

<style>
.plugin-card:hover {
    transform: translateY(-8px);
    box-shadow: 0 20px 40px rgba(255, 69, 0, 0.2), 0 20px 40px rgba(0, 170, 255, 0.1);
}

.plugin-card:hover .plugin-image img {
    transform: scale(1.05);
}

.plugin-card:hover > div:first-child {
    opacity: 1;
}

.plugin-card:hover .btn-hero {
    background: linear-gradient(90deg, var(--blue) 0%, var(--red) 100%);
    box-shadow: 0 15px 35px rgba(0, 170, 255, 0.4), 0 15px 35px rgba(255, 69, 0, 0.3);
}
</style>

<!-- Latest Plugins -->
<?php if (!empty($latest)): ?>
<section class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-20 border-t border-gray-800/50">
    <div class="flex items-center justify-between mb-10">
        <div>
            <h2 class="text-2xl sm:text-3xl font-bold text-white">Latest Plugins</h2>
            <p class="text-gray-400 mt-2">Recently added to the marketplace</p>
        </div>
    </div>
    <div class="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-6">
        <?php foreach ($latest as $plugin): ?>
        <a href="/plugin/<?= e($plugin['slug']) ?>" class="group bg-gray-900 border border-gray-800 rounded-2xl overflow-hidden hover:border-brand-500/50 transition-all hover:shadow-xl hover:shadow-brand-500/5">
            <div class="aspect-video bg-gray-800 relative overflow-hidden">
                <?php if ($plugin['featured_image']): ?>
                    <img src="/uploads/images/<?= e($plugin['featured_image']) ?>" alt="<?= e($plugin['name']) ?>" class="w-full h-full object-cover group-hover:scale-105 transition-transform duration-500">
                <?php else: ?>
                    <div class="w-full h-full flex items-center justify-center bg-gradient-to-br from-brand-600/20 to-purple-600/20">
                        <svg class="w-12 h-12 text-gray-600" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="1" d="M20 7l-8-4-8 4m16 0l-8 4m8-4v10l-8 4m0-10L4 7m8 4v10M4 7v10l8 4"/></svg>
                    </div>
                <?php endif; ?>
            </div>
            <div class="p-4">
                <h3 class="font-semibold text-white group-hover:text-brand-400 transition text-sm"><?= e($plugin['name']) ?></h3>
                <p class="text-xs text-gray-500 mt-1 line-clamp-2"><?= e($plugin['description']) ?></p>
                <div class="flex items-center justify-between mt-3 pt-3 border-t border-gray-800">
                    <?php if ($plugin['latest_version']): ?>
                        <span class="text-xs text-gray-500">v<?= e($plugin['latest_version']) ?></span>
                    <?php else: ?>
                        <span></span>
                    <?php endif; ?>
                    <?php if ($plugin['min_price']): ?>
                        <span class="text-brand-400 font-semibold text-sm"><?= format_price($plugin['min_price']) ?></span>
                    <?php else: ?>
                        <span class="text-emerald-400 font-semibold text-sm">Free</span>
                    <?php endif; ?>
                </div>
            </div>
        </a>
        <?php endforeach; ?>
    </div>
</section>
<?php endif; ?>

<!-- Newsletter Section -->
<section class="newsletter-section">
    <div class="vdj-container">
        <div class="newsletter-content" data-aos="zoom-in">
            <div class="newsletter-icon">
                <i class="fas fa-envelope"></i>
            </div>
            <h2 class="newsletter-title">
                STAY UPDATED WITH <span class="vdj-highlight-red">TELLYMEDIA</span>
            </h2>
            <p class="newsletter-subtitle">
                Get the latest plugin releases, exclusive deals, and DJ tips delivered to your inbox
            </p>
            <form class="newsletter-form" onsubmit="event.preventDefault(); alert('Newsletter signup coming soon!');">
                <input type="email" placeholder="Enter your email address" class="newsletter-input" required>
                <button type="submit" class="newsletter-btn">
                    <span>Subscribe</span>
                    <i class="fas fa-paper-plane"></i>
                </button>
            </form>
            <p class="newsletter-privacy">
                <i class="fas fa-lock"></i> We respect your privacy. Unsubscribe at any time.
            </p>
        </div>
    </div>
</section>

<!-- CTA Section -->
<?php if (!is_logged_in()): ?>
<section class="cta-section" data-aos="fade-up">
    <div class="vdj-container">
        <div class="cta-card">
            <div class="cta-pattern"></div>
            <div class="cta-content">
                <h2 class="cta-title">Ready to Transform Your DJ Sets?</h2>
                <p class="cta-subtitle">Create your account today and get instant access to premium plugins with secure licensing.</p>
                <div class="cta-buttons">
                    <a href="/register" class="vdj-btn vdj-btn-primary">
                        <span>Create Free Account</span>
                        <i class="fas fa-arrow-right"></i>
                    </a>
                    <a href="/plugins" class="vdj-btn vdj-btn-secondary">
                        <span>Browse Plugins</span>
                    </a>
                </div>
            </div>
        </div>
    </div>
</section>
<?php endif; ?>

<?php require BASE_PATH . '/includes/footer.php'; ?>
