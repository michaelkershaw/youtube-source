// Plugin Collection - Interactive Features

document.addEventListener('DOMContentLoaded', function() {
    const searchInput = document.getElementById('searchInput');
    const categoryFilter = document.getElementById('categoryFilter');
    const pluginList = document.getElementById('pluginList');
    const noResults = document.getElementById('noResults');
    let pluginCards = [];

    // Inline plugin data (fallback for local file access)
    const pluginsData = [
        {
            "folder": "tellymedia-reborn",
            "name": "TellyMedia Reborn v2",
            "displayName": "TellyMedia",
            "displayNameSuffix": "Reborn v2",
            "category": "video",
            "description": "Professional visual effects with 38+ advanced beat-reactive shaders, transparent overlays, and custom animations",
            "version": "2.0",
            "downloads": "15K+",
            "rating": "4.9",
            "status": "active",
            "coverImage": "Plugins/tellymedia-reborn/cover.png",
            "link": "Plugins/tellymedia-reborn/index.html"
        },
        {
            "folder": "youtube-source",
            "name": "YouTube Source",
            "displayName": "YouTube Source",
            "displayNameSuffix": "",
            "category": "video",
            "description": "Stream YouTube videos directly in VirtualDJ with seamless integration and real-time processing",
            "version": "1.5",
            "downloads": "",
            "rating": "",
            "status": "coming-soon",
            "coverImage": "Plugins/youtube-source/cover.png",
            "link": "Plugins/youtube-source/index.html"
        }
    ];

    // Load plugins dynamically from manifest
    async function loadPlugins() {
        let plugins = pluginsData; // Use inline data as default
        
        try {
            const response = await fetch('Plugins/plugins-manifest.json');
            if (response.ok) {
                plugins = await response.json();
            }
        } catch (error) {
            console.log('Using inline plugin data (fetch failed):', error.message);
        }
        
        // Clear existing static content
        pluginList.innerHTML = '';
        
        // Create plugin cards dynamically
        plugins.forEach(plugin => {
            const card = createPluginCard(plugin);
            pluginList.appendChild(card);
        });
        
        // Update plugin cards reference
        pluginCards = document.querySelectorAll('.plugin-card');
        
        // Initialize animations
        initializeCardAnimations();
    }

    function createPluginCard(plugin) {
        const card = document.createElement('div');
        card.className = 'plugin-card';
        card.setAttribute('data-category', plugin.category);
        
        // Badge
        let badgeClass = 'badge-soon';
        let badgeText = 'COMING SOON';
        if (plugin.status === 'active') {
            badgeClass = 'badge-active';
            badgeText = 'ACTIVE';
        } else if (plugin.status === 'beta') {
            badgeClass = 'badge-beta';
            badgeText = 'BETA';
        }
        
        // Build card HTML
        card.innerHTML = `
            ${plugin.coverImage ? `<img src="${plugin.coverImage}" alt="${plugin.name}" class="card-cover">` : ''}
            <span class="badge ${badgeClass}">${badgeText}</span>
            <h3 class="card-title">${plugin.displayNameSuffix ? 
                `<span class="name-telly">${plugin.displayName.split(' ')[0]}</span><span class="name-media">${plugin.displayName.split(' ')[1] || ''}</span> ${plugin.displayNameSuffix}` : 
                plugin.name}</h3>
            <p class="card-category">${getCategoryName(plugin.category)}</p>
            <p class="card-desc">${plugin.description}</p>
            <div class="card-stats">
                <span>v${plugin.version}</span>
            </div>
            ${plugin.status === 'active' ? 
                `<a href="${plugin.link}" class="btn-card">View Details</a>` : 
                `<button class="btn-card" disabled>${badgeText === 'BETA' ? 'Beta' : 'Coming Soon'}</button>`}
        `;
        
        return card;
    }

    function getCategoryName(category) {
        const categories = {
            'video': 'Video Effects',
            'audio': 'Audio Tools',
            'utility': 'Utilities'
        };
        return categories[category] || category;
    }

    function initializeCardAnimations() {
        const observerOptions = {
            threshold: 0.1,
            rootMargin: '0px 0px -50px 0px'
        };

        const observer = new IntersectionObserver((entries) => {
            entries.forEach(entry => {
                if (entry.isIntersecting) {
                    entry.target.style.opacity = '1';
                    entry.target.style.transform = 'translateY(0)';
                }
            });
        }, observerOptions);

        pluginCards.forEach(card => {
            card.style.opacity = '0';
            card.style.transform = 'translateY(20px)';
            card.style.transition = 'opacity 0.5s ease, transform 0.5s ease';
            observer.observe(card);
        });
    }

    // Load plugins on page load
    loadPlugins();

    // Filter function
    function filterPlugins() {
        const searchTerm = searchInput.value.toLowerCase();
        const selectedCategory = categoryFilter.value;
        let visibleCount = 0;

        pluginCards.forEach(card => {
            const title = card.querySelector('.card-title').textContent.toLowerCase();
            const description = card.querySelector('.card-desc').textContent.toLowerCase();
            const category = card.getAttribute('data-category');
            
            const matchesSearch = title.includes(searchTerm) || description.includes(searchTerm);
            const matchesCategory = selectedCategory === 'all' || category === selectedCategory;

            if (matchesSearch && matchesCategory) {
                card.style.display = 'block';
                visibleCount++;
            } else {
                card.style.display = 'none';
            }
        });

        // Show/hide no results message
        if (visibleCount === 0) {
            noResults.style.display = 'block';
            pluginsGrid.style.display = 'none';
        } else {
            noResults.style.display = 'none';
            pluginsGrid.style.display = 'grid';
        }
    }

    // Event listeners
    searchInput.addEventListener('input', filterPlugins);
    categoryFilter.addEventListener('change', filterPlugins);

    // Smooth scroll for navigation links
    document.querySelectorAll('a[href^="#"]').forEach(anchor => {
        anchor.addEventListener('click', function (e) {
            e.preventDefault();
            const target = document.querySelector(this.getAttribute('href'));
            if (target) {
                target.scrollIntoView({
                    behavior: 'smooth',
                    block: 'start'
                });
            }
        });
    });

    // Add animation on scroll
    const observerOptions = {
        threshold: 0.1,
        rootMargin: '0px 0px -50px 0px'
    };

    const observer = new IntersectionObserver((entries) => {
        entries.forEach(entry => {
            if (entry.isIntersecting) {
                entry.target.style.opacity = '1';
                entry.target.style.transform = 'translateY(0)';
            }
        });
    }, observerOptions);

    // Observe plugin cards
    pluginCards.forEach(card => {
        card.style.opacity = '0';
        card.style.transform = 'translateY(20px)';
        card.style.transition = 'opacity 0.5s ease, transform 0.5s ease';
        observer.observe(card);
    });

    // Image Slider Functionality
    function initSlider(containerClass, slideClass, dotClass) {
        const slides = document.querySelectorAll(slideClass);
        const dots = document.querySelectorAll(dotClass);
        let currentSlide = 0;

        if (slides.length === 0) return;

        function showSlide(index) {
            slides.forEach(slide => slide.classList.remove('active'));
            dots.forEach(dot => dot.classList.remove('active'));
            
            slides[index].classList.add('active');
            dots[index].classList.add('active');
        }

        function nextSlide() {
            currentSlide = (currentSlide + 1) % slides.length;
            showSlide(currentSlide);
        }

        // Dot click handlers
        dots.forEach((dot, index) => {
            dot.addEventListener('click', () => {
                currentSlide = index;
                showSlide(currentSlide);
            });
        });

        // Auto-advance every 4 seconds
        setInterval(nextSlide, 4000);
    }

    // Initialize featured slider
    initSlider('.featured-slider-container', '.featured-slide', '.featured-dot');

    });
