<?php
declare(strict_types=1);

require_once __DIR__ . '/config.php';
require_once __DIR__ . '/db.php';

$token = trim((string)($_GET['id'] ?? ''));
$ownerToken = trim((string)($_GET['owner'] ?? ''));
if ($token === '' || !preg_match('/^[a-f0-9]{24}$/', $token)) {
    http_response_code(404);
    echo 'Missing post ID';
    exit;
}

$db = getDB();
$stmt = $db->prepare('SELECT * FROM posts WHERE token = :token AND hidden = 0');
$stmt->execute([':token' => $token]);
$post = $stmt->fetch(PDO::FETCH_ASSOC);

if (!$post || ($post['visibility'] === VISIBILITY_PRIVATE && !ownerCanViewPost($db, $post, $ownerToken))) {
    http_response_code(404);
    htmlHeader('Not Found');
    echo '<div class="error">Post not found.</div>';
    htmlFooter();
    exit;
}

// Handle report
if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_POST['report'])) {
    $reason = mb_substr(trim((string)($_POST['reason'] ?? '')), 0, 500);
    $ip = clientIp();
    $existsStmt = $db->prepare('SELECT 1 FROM reports WHERE post_id = :pid AND reporter_ip = :ip LIMIT 1');
    $existsStmt->execute([':pid' => $post['id'], ':ip' => $ip]);
    if (!$existsStmt->fetchColumn()) {
        $rStmt = $db->prepare('INSERT INTO reports (post_id, reason, reporter_ip) VALUES (:pid, :reason, :ip)');
        $rStmt->execute([':pid' => $post['id'], ':reason' => $reason, ':ip' => $ip]);
        $db->prepare('UPDATE posts SET reported = reported + 1 WHERE id = :id')->execute([':id' => $post['id']]);
    }
    $reported = true;
}

$title = $post['title'] ?: 'Untitled';
htmlHeader($title);
?>

<div class="post-image">
    <a href="<?= h(mediaUrl($db, 'uploads', $post['filename'])) ?>">
        <img src="<?= h(mediaUrl($db, 'uploads', $post['filename'])) ?>" alt="<?= h($title) ?>" class="post-image">
    </a>
</div>

<div class="post-meta">
    <h2><?= h($title) ?></h2>
    <?php if ($post['description']): ?>
        <p><?= nl2br(h($post['description'])) ?></p>
    <?php endif; ?>

    <table>
        <?php if ($post['avatar_name']): ?><tr><th>Avatar</th><td><?= h($post['avatar_name']) ?></td></tr><?php endif; ?>
        <?php if ($post['grid_name']): ?><tr><th>Grid</th><td><?= h($post['grid_name']) ?></td></tr><?php endif; ?>
        <tr><th>Added</th><td><?= h(gmdate('Y-m-d H:i:s', strtotime($post['created_at']))) ?> UTC</td></tr>
    </table>

    <?php if (isset($reported) && $reported): ?>
        <div class="notice">Thank you. This post has been reported.</div>
    <?php else: ?>
        <form method="post" style="margin-top:1em">
            <input type="hidden" name="report" value="1">
            <label>Report this post:<br>
                <input type="text" name="reason" placeholder="Reason (optional)" size="40" maxlength="500">
            </label>
            <button type="submit" class="btn btn-small">Report</button>
        </form>
    <?php endif; ?>
</div>

<p><a href="<?= BASE_URL ?>/">&laquo; Back to feed</a></p>

<?php htmlFooter();

function ownerCanViewPost(PDO $db, array $post, string $ownerToken): bool
{
    if ($ownerToken === '' || !preg_match('/^[a-f0-9]{48}$/', $ownerToken)) {
        return false;
    }

    if (hash_equals((string)$post['owner_token'], $ownerToken)) {
        return true;
    }

    $userUuid = (string)($post['user_uuid'] ?? '');
    if ($userUuid === '') {
        return false;
    }

    $stmt = $db->prepare('SELECT 1 FROM posts WHERE owner_token = :owner_token AND user_uuid = :user_uuid AND hidden = 0 LIMIT 1');
    $stmt->execute([':owner_token' => $ownerToken, ':user_uuid' => $userUuid]);
    return (bool)$stmt->fetchColumn();
}
