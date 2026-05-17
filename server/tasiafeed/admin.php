<?php
declare(strict_types=1);

require_once __DIR__ . '/config.php';
require_once __DIR__ . '/db.php';

session_start();

function csrfToken(): string
{
    if (empty($_SESSION['csrf_token'])) {
        $_SESSION['csrf_token'] = bin2hex(random_bytes(16));
    }
    return (string)$_SESSION['csrf_token'];
}

function requireCsrf(): void
{
    $token = (string)($_POST['csrf_token'] ?? '');
    if ($token === '' || !hash_equals((string)($_SESSION['csrf_token'] ?? ''), $token)) {
        http_response_code(403);
        htmlHeader('Forbidden');
        echo '<div class="error">Invalid admin request.</div>';
        htmlFooter();
        exit;
    }
}

function redirectAdmin(): void
{
    header('Location: admin.php', true, 303);
    exit;
}

$adminConfigured = ADMIN_SECRET !== '';
$adminAuthenticated = $adminConfigured && !empty($_SESSION['tasiafeed_admin']);
$authError = '';

if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_POST['login'])) {
    if (!$adminConfigured) {
        $authError = 'Admin is disabled until TASIAFEED_ADMIN_SECRET is configured.';
    } else {
        $secret = (string)($_POST['secret'] ?? '');
        if ($secret !== '' && hash_equals(ADMIN_SECRET, $secret)) {
            session_regenerate_id(true);
            $_SESSION['tasiafeed_admin'] = true;
            csrfToken();
            redirectAdmin();
        }
        $authError = 'Invalid admin secret.';
    }
}

if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_POST['logout'])) {
    requireCsrf();
    $_SESSION = [];
    session_destroy();
    redirectAdmin();
}

if (!$adminAuthenticated) {
    htmlHeader('Admin Login');
    if (!$adminConfigured): ?>
        <div class="error">Admin is disabled. Set <code>TASIAFEED_ADMIN_SECRET</code> in the environment or <code>config.local.php</code>.</div>
    <?php endif; ?>
    <?php if ($authError !== ''): ?>
        <div class="error"><?= h($authError) ?></div>
    <?php endif; ?>
    <form method="post" action="admin.php">
        <input type="hidden" name="login" value="1">
        <label>Admin Secret:<br>
            <input type="password" name="secret" size="40" autocomplete="current-password">
        </label>
        <button type="submit" class="btn" <?= $adminConfigured ? '' : 'disabled' ?>>Login</button>
    </form>
    <?php
    htmlFooter();
    exit;
}

$db = getDB();

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    requireCsrf();
    $action = (string)($_POST['action'] ?? '');
    $postId = (int)($_POST['post_id'] ?? 0);

    if ($postId > 0 && in_array($action, ['hide', 'unhide'], true)) {
        $hidden = $action === 'hide' ? 1 : 0;
        $db->prepare('UPDATE posts SET hidden = :hidden WHERE id = :id')
            ->execute([':hidden' => $hidden, ':id' => $postId]);
    }

    if ($postId > 0 && $action === 'delete') {
        $stmt = $db->prepare('SELECT filename, thumbname FROM posts WHERE id = :id');
        $stmt->execute([':id' => $postId]);
        $post = $stmt->fetch(PDO::FETCH_ASSOC);
        if ($post) {
            foreach ([STORAGE_DIR . '/' . $post['filename'], THUMBS_DIR . '/' . $post['thumbname']] as $file) {
                if (is_file($file)) {
                    @unlink($file);
                }
            }
            $db->prepare('DELETE FROM reports WHERE post_id = :id')->execute([':id' => $postId]);
            $db->prepare('DELETE FROM posts WHERE id = :id')->execute([':id' => $postId]);
        }
    }

    if ($action === 'save_storage') {
        $backend = (string)($_POST['storage_backend'] ?? 'local');
        if (!in_array($backend, ['local', 'webdav'], true)) {
            $backend = 'local';
        }

        setSetting($db, 'storage_backend', $backend);
        setSetting($db, 'webdav_url', mb_substr(trim((string)($_POST['webdav_url'] ?? '')), 0, 500));
        setSetting($db, 'webdav_public_url', mb_substr(trim((string)($_POST['webdav_public_url'] ?? '')), 0, 500));
        setSetting($db, 'webdav_username', mb_substr(trim((string)($_POST['webdav_username'] ?? '')), 0, 255));

        $password = (string)($_POST['webdav_password'] ?? '');
        if ($password !== '') {
            setSetting($db, 'webdav_password', $password);
        }
    }

    redirectAdmin();
}

$totalPosts = (int)$db->query('SELECT COUNT(*) FROM posts')->fetchColumn();
$reportedPosts = (int)$db->query('SELECT COUNT(*) FROM posts WHERE reported > 0')->fetchColumn();
$hiddenPosts = (int)$db->query('SELECT COUNT(*) FROM posts WHERE hidden = 1')->fetchColumn();
$storageBackend = getSetting($db, 'storage_backend', 'local');
$webdavUrl = getSetting($db, 'webdav_url');
$webdavPublicUrl = getSetting($db, 'webdav_public_url');
$webdavUsername = getSetting($db, 'webdav_username');
$webdavPasswordSet = getSetting($db, 'webdav_password') !== '';

htmlHeader('Admin Panel');
$csrf = csrfToken();
?>

<form method="post" action="admin.php" style="float:right">
    <input type="hidden" name="csrf_token" value="<?= h($csrf) ?>">
    <button type="submit" name="logout" value="1" class="btn btn-small">Logout</button>
</form>

<div class="admin-stats" style="margin-bottom:1em">
    <strong>Stats:</strong>
    Total: <?= $totalPosts ?> |
    Reported: <?= $reportedPosts ?> |
    Hidden: <?= $hiddenPosts ?>
</div>

<section class="settings-panel">
    <h3>Storage Settings</h3>
    <p class="notice">Local storage keeps a server copy. If WebDAV is selected, uploads are also pushed to WebDAV and public image URLs use the WebDAV public URL when provided.</p>
    <form method="post" action="admin.php">
        <input type="hidden" name="csrf_token" value="<?= h($csrf) ?>">
        <input type="hidden" name="action" value="save_storage">

        <label>Storage backend<br>
            <select name="storage_backend">
                <option value="local" <?= $storageBackend === 'local' ? 'selected' : '' ?>>Local server storage</option>
                <option value="webdav" <?= $storageBackend === 'webdav' ? 'selected' : '' ?>>WebDAV</option>
            </select>
        </label>

        <label>WebDAV URL<br>
            <input type="url" name="webdav_url" value="<?= h($webdavUrl) ?>" placeholder="https://example.com/remote.php/dav/files/user/tasiafeed">
        </label>

        <label>WebDAV public URL<br>
            <input type="url" name="webdav_public_url" value="<?= h($webdavPublicUrl) ?>" placeholder="https://cdn.example.com/tasiafeed">
        </label>

        <label>WebDAV username<br>
            <input type="text" name="webdav_username" value="<?= h($webdavUsername) ?>" autocomplete="username">
        </label>

        <label>WebDAV password / app password<br>
            <input type="password" name="webdav_password" placeholder="<?= $webdavPasswordSet ? 'Password is set; leave blank to keep it' : 'Not set' ?>" autocomplete="new-password">
        </label>

        <button type="submit" class="btn">Save storage settings</button>
    </form>
</section>

<h3>Reported Posts</h3>
<?php
$reported = $db->query('
    SELECT p.*, r.reason, r.reporter_ip, r.created_at AS report_date
    FROM posts p
    JOIN reports r ON r.post_id = p.id
    ORDER BY r.created_at DESC
    LIMIT 50
')->fetchAll(PDO::FETCH_ASSOC);

if (empty($reported)): ?>
<p>No reported posts.</p>
<?php else: ?>
<table class="admin-table">
<tr><th>ID</th><th>Token</th><th>Title</th><th>Avatar</th><th>Reason</th><th>Reporter</th><th>Reported</th><th>Hidden</th><th>Actions</th></tr>
<?php foreach ($reported as $p): ?>
<tr>
    <td><?= (int)$p['id'] ?></td>
    <td><a href="post.php?id=<?= rawurlencode($p['token']) ?>"><?= h(mb_substr($p['token'], 0, 12)) ?>...</a></td>
    <td><?= h(mb_substr($p['title'] ?: 'Untitled', 0, 30)) ?></td>
    <td><?= h($p['avatar_name']) ?></td>
    <td><?= h($p['reason']) ?></td>
    <td><?= h($p['reporter_ip']) ?></td>
    <td><?= h(gmdate('Y-m-d', strtotime($p['report_date']))) ?></td>
    <td><?= $p['hidden'] ? 'Yes' : 'No' ?></td>
    <td><?php renderActions((int)$p['id'], (bool)$p['hidden'], $csrf); ?></td>
</tr>
<?php endforeach; ?>
</table>
<?php endif; ?>

<h3>All Posts</h3>
<?php $allPosts = $db->query('SELECT id, token, title, avatar_name, hidden, reported, created_at FROM posts ORDER BY created_at DESC LIMIT 50')->fetchAll(PDO::FETCH_ASSOC); ?>
<table class="admin-table">
<tr><th>ID</th><th>Token</th><th>Title</th><th>Avatar</th><th>Hidden</th><th>Reported</th><th>Date</th><th>Actions</th></tr>
<?php foreach ($allPosts as $p): ?>
<tr>
    <td><?= (int)$p['id'] ?></td>
    <td><a href="post.php?id=<?= rawurlencode($p['token']) ?>"><?= h(mb_substr($p['token'], 0, 12)) ?>...</a></td>
    <td><?= h(mb_substr($p['title'] ?: 'Untitled', 0, 30)) ?></td>
    <td><?= h($p['avatar_name']) ?></td>
    <td><?= $p['hidden'] ? 'Yes' : 'No' ?></td>
    <td><?= (int)$p['reported'] ?></td>
    <td><?= h(gmdate('Y-m-d', strtotime($p['created_at']))) ?></td>
    <td><?php renderActions((int)$p['id'], (bool)$p['hidden'], $csrf); ?></td>
</tr>
<?php endforeach; ?>
</table>

<?php
htmlFooter();

function renderActions(int $postId, bool $hidden, string $csrf): void
{
    ?>
    <form method="post" action="admin.php" style="display:inline">
        <input type="hidden" name="csrf_token" value="<?= h($csrf) ?>">
        <input type="hidden" name="post_id" value="<?= $postId ?>">
        <?php if ($hidden): ?>
            <button type="submit" name="action" value="unhide" class="btn btn-small">Unhide</button>
        <?php else: ?>
            <button type="submit" name="action" value="hide" class="btn btn-small">Hide</button>
        <?php endif; ?>
        <button type="submit" name="action" value="delete" class="btn btn-small" onclick="return confirm('Delete this post permanently?')">Delete</button>
    </form>
    <?php
}
