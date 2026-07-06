<?php
/**
 * License Key Generator Service
 * Supports plugin-specific formats
 */

class LicenseKeyGenerator {

    /**
     * Generate a unique license key for a plugin
     */
    public static function generate(int $pluginId): string {
        $format = Database::fetch(
            "SELECT * FROM license_formats WHERE plugin_id = ? AND is_active = 1", 
            [$pluginId]
        );

        do {
            if ($format) {
                $key = self::generateFromFormat($format);
            } else {
                $key = self::generateDefault();
            }
        } while (Database::count('licenses', 'license_key = ?', [$key]) > 0);

        return $key;
    }

    /**
     * Generate from plugin-specific format
     * Pattern chars: X = alphanumeric, # = digit, A = letter
     */
    private static function generateFromFormat(array $format): string {
        $pattern = $format['format_pattern'];
        $prefix = $format['prefix'] ?? '';
        $separator = $format['separator'] ?? '-';

        $key = '';
        for ($i = 0; $i < strlen($pattern); $i++) {
            $char = $pattern[$i];
            if ($char === 'X') {
                $key .= self::randomAlphaNum();
            } elseif ($char === '#') {
                $key .= rand(0, 9);
            } elseif ($char === 'A') {
                $key .= chr(rand(65, 90));
            } else {
                $key .= $char;
            }
        }

        if ($prefix) {
            $key = $prefix . $separator . $key;
        }

        return strtoupper($key);
    }

    /**
     * Default format: XXXX-XXXX-XXXX-XXXX
     */
    private static function generateDefault(): string {
        $segments = [];
        for ($i = 0; $i < 4; $i++) {
            $segment = '';
            for ($j = 0; $j < 4; $j++) {
                $segment .= self::randomAlphaNum();
            }
            $segments[] = $segment;
        }
        return strtoupper(implode('-', $segments));
    }

    /**
     * Random alphanumeric (no ambiguous chars 0/O, 1/I/L)
     */
    private static function randomAlphaNum(): string {
        $chars = '23456789ABCDEFGHJKMNPQRSTUVWXYZ';
        return $chars[rand(0, strlen($chars) - 1)];
    }

    /**
     * Calculate expiration date from license type
     */
    public static function calculateExpiration(string $licenseType, ?int $customDays = null): ?string {
        return match($licenseType) {
            'monthly'     => date('Y-m-d H:i:s', strtotime('+1 month')),
            'yearly'      => date('Y-m-d H:i:s', strtotime('+1 year')),
            'lifetime'    => null,
            'custom_days' => $customDays 
                ? date('Y-m-d H:i:s', strtotime("+{$customDays} days")) 
                : date('Y-m-d H:i:s', strtotime('+1 month')),
            default       => date('Y-m-d H:i:s', strtotime('+1 month')),
        };
    }

    /**
     * Check if a license is active
     */
    public static function isActive(array $license): bool {
        if ($license['status'] !== 'active') return false;
        if ($license['expires_at'] && strtotime($license['expires_at']) < time()) return false;
        return true;
    }

    /**
     * Check if a license can accept new activations
     */
    public static function canActivate(array $license): bool {
        return $license['current_activations'] < $license['max_activations'];
    }
}
