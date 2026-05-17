<?php
declare(strict_types=1);

// TasiaFeed Configuration
// Upload this directory to: https://apps.easierit.org/igrid/feed/

$localConfig = __DIR__ . '/config.local.php';
if (is_file($localConfig)) {
    require_once $localConfig;
}

function tasiafeedConfigValue(string $name, string $default = ''): string
{
    if (defined($name)) {
        return (string)constant($name);
    }

    $value = getenv($name);
    return ($value === false) ? $default : (string)$value;
}

// --- Paths ---
define('STORAGE_DIR', __DIR__ . '/uploads');
define('THUMBS_DIR', __DIR__ . '/thumbs');
define('DB_PATH', __DIR__ . '/feed.sqlite');

// --- URLs (no trailing slashes) ---
define('BASE_URL', 'https://apps.easierit.org/igrid/feed');
define('UPLOADS_URL', BASE_URL . '/uploads');
define('THUMBS_URL', BASE_URL . '/thumbs');

// --- Limits ---
define('MAX_FILE_SIZE', 20 * 1024 * 1024);       // 20 MB
define('ALLOWED_MIME_TYPES', ['image/jpeg', 'image/png', 'image/webp']);
define('ALLOWED_EXTENSIONS', ['jpg', 'jpeg', 'png', 'webp']);
define('THUMB_WIDTH', 280);
define('THUMB_HEIGHT', 280);
define('ITEMS_PER_PAGE', 30);

// --- Auth ---
// Set with TASIAFEED_ADMIN_SECRET environment variable or config.local.php.
// Keep config.local.php out of git.
// Temporary default requested by Mom; change in production.
define('ADMIN_SECRET', tasiafeedConfigValue('TASIAFEED_ADMIN_SECRET', 'tasia123'));

// --- Post visibility ---
define('VISIBILITY_PUBLIC', 'public');
define('VISIBILITY_UNLISTED', 'unlisted');
define('VISIBILITY_PRIVATE', 'private');
