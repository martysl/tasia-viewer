<?php
declare(strict_types=1);

require_once __DIR__ . '/config.php';

function getDB(): PDO
{
    static $db = null;
    if ($db === null) {
        $db = new PDO('sqlite:' . DB_PATH);
        $db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
        $db->exec('PRAGMA journal_mode=WAL');
        $db->exec('PRAGMA foreign_keys=ON');
        initDB($db);
    }
    return $db;
}

function initDB(PDO $db): void
{
    $db->exec('
        CREATE TABLE IF NOT EXISTS posts (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            token       TEXT NOT NULL UNIQUE,
            filename    TEXT NOT NULL,
            thumbname   TEXT NOT NULL,
            title       TEXT NOT NULL DEFAULT "",
            description TEXT NOT NULL DEFAULT "",
            visibility  TEXT NOT NULL DEFAULT "public",
            maturity    TEXT NOT NULL DEFAULT "general",
            avatar_name TEXT NOT NULL DEFAULT "",
            grid_name   TEXT NOT NULL DEFAULT "",
            region_name TEXT NOT NULL DEFAULT "",
            position    TEXT NOT NULL DEFAULT "",
            viewer_ver  TEXT NOT NULL DEFAULT "",
            commit_sha  TEXT NOT NULL DEFAULT "",
            build_num   TEXT NOT NULL DEFAULT "",
            user_uuid   TEXT NOT NULL DEFAULT "",
            owner_token TEXT NOT NULL DEFAULT "",
            ip_addr     TEXT NOT NULL DEFAULT "",
            user_agent  TEXT NOT NULL DEFAULT "",
            created_at  TEXT NOT NULL DEFAULT (CURRENT_TIMESTAMP),
            reported    INTEGER NOT NULL DEFAULT 0,
            hidden      INTEGER NOT NULL DEFAULT 0
        )
    ');

    $db->exec('
        CREATE TABLE IF NOT EXISTS reports (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            post_id     INTEGER NOT NULL REFERENCES posts(id),
            reason      TEXT NOT NULL DEFAULT "",
            reporter_ip TEXT NOT NULL DEFAULT "",
            created_at  TEXT NOT NULL DEFAULT (CURRENT_TIMESTAMP)
        )
    ');

    ensureColumn($db, 'posts', 'owner_token', 'TEXT NOT NULL DEFAULT ""');

    $db->exec('CREATE INDEX IF NOT EXISTS idx_posts_public ON posts (visibility, hidden, created_at)');
    $db->exec('CREATE INDEX IF NOT EXISTS idx_posts_token ON posts (token)');
    $db->exec('CREATE UNIQUE INDEX IF NOT EXISTS idx_posts_owner_token ON posts (owner_token) WHERE owner_token != ""');
    $db->exec('CREATE INDEX IF NOT EXISTS idx_reports_post ON reports (post_id)');
    $db->exec('DELETE FROM reports WHERE rowid NOT IN (SELECT MIN(rowid) FROM reports GROUP BY post_id, reporter_ip)');
    $db->exec('CREATE UNIQUE INDEX IF NOT EXISTS idx_reports_post_ip ON reports (post_id, reporter_ip)');

    $db->exec('
        CREATE TABLE IF NOT EXISTS settings (
            name       TEXT PRIMARY KEY,
            value      TEXT NOT NULL DEFAULT "",
            updated_at TEXT NOT NULL DEFAULT (CURRENT_TIMESTAMP)
        )
    ');
}

function ensureColumn(PDO $db, string $table, string $column, string $definition): void
{
    $stmt = $db->query('PRAGMA table_info(' . $table . ')');
    $columns = $stmt->fetchAll(PDO::FETCH_ASSOC);
    foreach ($columns as $info) {
        if (($info['name'] ?? '') === $column) {
            return;
        }
    }
    $db->exec('ALTER TABLE ' . $table . ' ADD COLUMN ' . $column . ' ' . $definition);
}

function generateToken(): string
{
    return bin2hex(random_bytes(12));
}

function generateOwnerToken(): string
{
    return bin2hex(random_bytes(24));
}

function jsonResponse($data, int $code = 200)
{
    http_response_code($code);
    header('Content-Type: application/json; charset=utf-8');
    echo json_encode($data, JSON_UNESCAPED_SLASHES | JSON_UNESCAPED_UNICODE) . "\n";
    exit;
}

function h(?string $value): string
{
    return htmlspecialchars((string)$value, ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8');
}

function publicFileUrl(string $baseUrl, string $relativePath): string
{
    $parts = array_map('rawurlencode', explode('/', str_replace('\\', '/', $relativePath)));
    return rtrim($baseUrl, '/') . '/' . implode('/', $parts);
}

function mediaUrl(PDO $db, string $kind, string $relativePath): string
{
    $localBase = $kind === 'thumbs' ? THUMBS_URL : UPLOADS_URL;
    if (getSetting($db, 'storage_backend', 'local') === 'webdav') {
        $publicBase = getSetting($db, 'webdav_public_url');
        if ($publicBase !== '') {
            return publicFileUrl(rtrim($publicBase, '/') . '/' . $kind, $relativePath);
        }
    }
    return publicFileUrl($localBase, $relativePath);
}

function clientIp(): string
{
    return mb_substr((string)($_SERVER['REMOTE_ADDR'] ?? ''), 0, 64);
}

function getSetting(PDO $db, string $name, string $default = ''): string
{
    $stmt = $db->prepare('SELECT value FROM settings WHERE name = :name');
    $stmt->execute([':name' => $name]);
    $value = $stmt->fetchColumn();
    return $value === false ? $default : (string)$value;
}

function setSetting(PDO $db, string $name, string $value): void
{
    $stmt = $db->prepare('
        INSERT INTO settings (name, value, updated_at)
        VALUES (:name, :value, CURRENT_TIMESTAMP)
        ON CONFLICT(name) DO UPDATE SET value = excluded.value, updated_at = CURRENT_TIMESTAMP
    ');
    $stmt->execute([':name' => $name, ':value' => $value]);
}

function htmlHeader(string $title): void
{
    header('Content-Type: text/html; charset=utf-8');
    ?><!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title><?= htmlspecialchars($title) ?> - TasiaFeed</title>
<style>
*, *::before, *::after { box-sizing: border-box; }
body {
    font-family: "Trebuchet MS", "Segoe UI", -apple-system, BlinkMacSystemFont, sans-serif;
    max-width: 1100px; margin: 0 auto; padding: 1.25em;
    background: radial-gradient(circle at top, #ffe5f1 0, #fff8fb 42%, #f7edf2 100%);
    color: #4b3440;
}
h1 { color: #e74c8b; text-align:center; font-size:2.4em; text-shadow: 0 2px 0 #fff; }
a { color: #e74c8b; text-decoration: none; }
a:hover { text-decoration: underline; }
.book-page { background:#fffafc; border:1px solid #ffd0e2; border-radius:22px; padding:1.25em; box-shadow:0 18px 45px rgba(185,74,124,.18), inset 0 0 0 8px #fff1f7; }
.filters { background:#fff; border:1px dashed #f4a6c7; border-radius:16px; padding:1em; margin-bottom:1em; display:flex; gap:.75em; flex-wrap:wrap; align-items:end; }
.filters label { font-weight:600; font-size:.9em; }
.filters select, .filters input { padding:.45em; border:1px solid #f0b5cf; border-radius:8px; background:#fffafe; }
.view-switch { text-align:center; margin:.5em 0 1em; }
.folder-grid { display:grid; grid-template-columns:repeat(auto-fill,minmax(220px,1fr)); gap:1em; margin:1em 0; }
.folder-card { display:block; background:linear-gradient(180deg,#fff 0,#ffeaf3 100%); border:1px solid #ffc2dc; border-radius:18px; padding:1em; box-shadow:0 8px 20px rgba(160,72,112,.12); min-height:110px; }
.folder-card:hover { text-decoration:none; transform:translateY(-2px); }
.folder-card .icon { font-size:2em; }
.folder-card .name { display:block; color:#7b2f55; font-weight:700; margin-top:.25em; }
.folder-card .count { display:block; color:#806070; font-size:.9em; margin-top:.25em; }
.breadcrumb { margin:.5em 0 1em; color:#806070; }
.group-title { margin:1.1em 0 .55em; padding:.45em .7em; background:#ffe4ef; border-left:6px solid #e74c8b; border-radius:10px; color:#7b2f55; }
.feed { display: grid; grid-template-columns: repeat(auto-fill, minmax(210px, 1fr)); gap: 1em; }
.feed-item { background: #fff; border-radius: 18px; overflow: hidden; box-shadow: 0 8px 20px rgba(160,72,112,0.14); border:1px solid #ffd4e5; transform: rotate(-.25deg); }
.feed-item:nth-child(even) { transform: rotate(.25deg); }
.feed-item img { width: 100%; height: 210px; object-fit: cover; display: block; }
.feed-item .info { padding: 0.75em; }
.feed-item .info .title { font-weight: 700; }
.feed-item .info .meta { font-size: 0.85em; color: #806070; }
.pagination { text-align: center; margin: 1em 0; }
.pagination a { display: inline-block; padding: 0.5em 1em; margin: 0 0.25em; background: #fff; border-radius: 4px; }
.post-image { max-width: 100%; height: auto; border-radius: 8px; }
.post-meta { background: #fff; padding: 1em; border-radius: 16px; margin-top: 1em; border:1px solid #ffd4e5; }
.btn { display: inline-block; padding: 0.5em 1em; background: #e74c8b; color: #fff; border: none; border-radius: 4px; cursor: pointer; text-decoration: none; font-size: 0.9em; }
.btn:hover { background: #d0407a; text-decoration: none; }
.btn-small { padding: 0.3em 0.6em; font-size: 0.8em; }
.admin-table { width: 100%; border-collapse: collapse; background: #fff; }
.admin-table th, .admin-table td { padding: 0.5em; border: 1px solid #ddd; text-align: left; }
.admin-table th { background: #f0f0f0; }
.settings-panel { background:#fff; border:1px solid #ffd4e5; border-radius:16px; padding:1em; margin:1em 0; }
.settings-panel input, .settings-panel select { width:100%; max-width:520px; padding:.45em; margin:.2em 0 .7em; border:1px solid #f0b5cf; border-radius:8px; }
.notice { background: #fff3cd; border: 1px solid #ffeeba; padding: 1em; border-radius: 4px; margin: 1em 0; }
.error { background: #f8d7da; border: 1px solid #f5c6cb; padding: 1em; border-radius: 4px; margin: 1em 0; }
</style>
</head>
<body>
<h1><a href="<?= BASE_URL ?>/">💕 TasiaFeed</a></h1>
<main class="book-page">
<?php
}

function htmlFooter(): void
{
    echo '</main>';
    echo '<p style="margin-top:2em;font-size:0.85em;color:#b07a91;text-align:center">TasiaFeed &mdash; part of Tasia Viewer</p>';
    echo '</body></html>';
}
