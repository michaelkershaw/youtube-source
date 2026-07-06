# Plugin Marketplace

A complete plugin marketplace and licensing system built with **pure PHP + MySQL + Tailwind CSS** (CDN). No frameworks, no Node.js, no build tools required.

## Features

- **User Accounts** - Register, login, password reset, profile management
- **Plugin Marketplace** - Browse, search, filter by category, detailed plugin pages
- **Payment System** - Checkout flow with mock payments (ready for Stripe/PayPal integration)
- **License System** - Flexible key formats, expiration tracking, per-plugin formats
- **Device Activation** - Track and limit activations per license (e.g. 3 PCs), user can deactivate devices
- **Secure Downloads** - Token-based download system with logging
- **Admin Panel** - Full CRUD for plugins, users, orders, licenses
- **Plugin Versions** - Upload and manage multiple versions with changelogs
- **License API** - REST endpoint for license verification from plugins
- **Multi-Domain** - Dynamic base URL detection, works on any domain

## Requirements

- PHP 7.4+ (PHP 8.x recommended)
- MySQL 5.7+ or MariaDB 10.3+
- Apache with mod_rewrite (or nginx with equivalent rewrite rules)

## Installation

1. **Clone/copy** the project to your web server's document root (or a subdirectory)

2. **Configure Apache** - Ensure `mod_rewrite` is enabled and `AllowOverride All` is set for the directory

3. **Edit `config.php`** - Update database credentials:
   ```php
   define('DB_HOST', '127.0.0.1');
   define('DB_NAME', 'plugin_marketplace');
   define('DB_USER', 'root');
   define('DB_PASS', '');
   ```

4. **Run the installer** - Visit `http://yoursite.com/install.php` in your browser, or run:
   ```bash
   php install.php
   ```
   This creates the database, tables, and demo data.

5. **Delete `install.php`** after setup for security.

## Default Logins

| Role  | Email              | Password  |
|-------|--------------------|-----------|
| Admin | admin@example.com  | admin123  |
| User  | user@example.com   | user123   |

## Project Structure

```
plugin-marketplace/
├── config.php              # Configuration (DB, paths, constants)
├── index.php               # Front controller / router
├── .htaccess               # URL rewriting
├── install.php             # Database setup & seeder
├── includes/
│   ├── db.php              # PDO database class
│   ├── auth.php            # Authentication & CSRF helpers
│   ├── helpers.php         # Utility functions
│   ├── license.php         # License key generator
│   ├── header.php          # Public layout header
│   ├── footer.php          # Public layout footer
│   ├── admin-header.php    # Admin layout header
│   └── admin-footer.php    # Admin layout footer
├── pages/
│   ├── home.php            # Homepage
│   ├── plugins.php         # Plugin listing
│   ├── plugin-detail.php   # Plugin detail page
│   ├── dashboard.php       # User dashboard
│   ├── profile.php         # User profile settings
│   ├── checkout.php        # Checkout page
│   ├── checkout-success.php# Order confirmation
│   ├── download.php        # Secure download handler
│   ├── deactivate-device.php
│   ├── 404.php
│   ├── auth/
│   │   ├── login.php
│   │   ├── register.php
│   │   ├── logout.php
│   │   ├── forgot-password.php
│   │   └── reset-password.php
│   ├── api/
│   │   ├── verify-license.php
│   │   └── deactivate-device.php
│   └── admin/
│       ├── dashboard.php
│       ├── plugins.php
│       ├── plugin-create.php
│       ├── plugin-edit.php
│       ├── plugin-view.php
│       ├── plugin-delete.php
│       ├── plugin-add-version.php
│       ├── plugin-add-image.php
│       ├── plugin-delete-image.php
│       ├── users.php
│       ├── user-view.php
│       ├── user-edit.php
│       ├── orders.php
│       ├── order-view.php
│       ├── order-refund.php
│       ├── licenses.php
│       ├── license-view.php
│       ├── license-create.php
│       └── license-action.php
├── database/
│   └── schema.sql          # Full database schema
└── uploads/
    ├── plugins/            # Plugin files (protected)
    └── images/             # Plugin images (public)
```

## API Endpoints

### Verify License
```
POST /api/verify-license
Content-Type: application/json

{
    "license_key": "XXXX-XXXX-XXXX-XXXX",
    "device_id": "unique-device-identifier",
    "device_name": "My PC"
}
```

### Deactivate Device
```
POST /api/deactivate-device
Content-Type: application/json

{
    "license_key": "XXXX-XXXX-XXXX-XXXX",
    "device_id": "unique-device-identifier"
}
```

## Security Notes

- Passwords are hashed with `password_hash()` (bcrypt)
- CSRF protection on all forms
- Prepared statements for all SQL queries (no SQL injection)
- Plugin files are stored outside web root access (protected by .htaccess)
- Session security: httponly cookies, strict mode
- Input validation and output escaping throughout

## Customization

- **Branding**: Edit `SITE_NAME` in `config.php`
- **Colors**: Modify the Tailwind config in `includes/header.php`
- **Payment**: Replace the mock payment in `pages/checkout.php` with Stripe/PayPal
- **Multi-domain**: Just deploy to any domain - URL detection is automatic
