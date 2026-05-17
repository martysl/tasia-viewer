<?php
declare(strict_types=1);

require_once __DIR__ . '/config.php';
require_once __DIR__ . '/db.php';

$ownerToken = trim((string)($_GET['token'] ?? $_POST['token'] ?? ''));
if ($ownerToken === '' || !preg_match('/^[a-f0-9]{48}$/', $ownerToken)) {
    ownerNotFound();
}

$db = getDB();
$stmt = $db->prepare('SELECT * FROM posts WHERE owner_token = :owner_token AND hidden = 0');
$stmt->execute([':owner_token' => $ownerToken]);
$ownerPost = $stmt->fetch(PDO::FETCH_ASSOC);

if (!$ownerPost) {
    ownerNotFound();
}

$ownerUserUuid = (string)($ownerPost['user_uuid'] ?? '');
$message = '';

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $action = (string)($_POST['action'] ?? '');
    $postId = (int)($_POST['post_id'] ?? 0);

    $target = findOwnedPost($db, $postId, $ownerPost, $ownerUserUuid);
    if (!$target) {
        http_response_code(403);
        htmlHeader('Forbidden');
        echo '<div class="error">This owner link cannot manage that post.</div>';
        htmlFooter();
        exit;
    }

    if ($action === 'visibility') {
        $visibility = (string)($_POST['visibility'] ?? VISIBILITY_PUBLIC);
        if (in_array($visibility, [VISIBILITY_PUBLIC, VISIBILITY_UNLISTED, VISIBILITY_PRIVATE], true)) {
            $db->prepare('UPDATE posts SET visibility = :visibility WHERE id = :id')
                ->execute([':visibility' => $visibility, ':id' => $target['id']]);
            $message = 'Visibility updated.';
        }
    }

    if ($action === 'delete') {
        foreach ([STORAGE_DIR . '/' . $target['filename'], THUMBS_DIR . '/' . $target['thumbname']] as $file) {
            if (is_file($file)) {
                @unlink($file);
            }
        }
        $db->prepare('DELETE FROM reports WHERE post_id = :id')->execute([':id' => $target['id']]);
        $db->prepare('DELETE FROM posts WHERE id = :id')->execute([':id' => $target['id']]);
        $message = 'Post deleted.';
    }
}

$posts = loadOwnedPosts($db, $ownerPost, $ownerUserUuid);

htmlHeader('Manage Uploads');
?>

<?php if ($message !== ''): ?>
    <div class="notice"><?= h($message) ?></div>
<?php endif; ?>

<h2>Your upload dashboard</h2>
<p class="notice">Keep this owner link private. It lets you manage uploads from this viewer user.</p>

<?php if (empty($posts)): ?>
    <div class="notice">No uploads found for this owner link.</div>
<?php else: ?>
    <div class="feed">
    <?php foreach ($posts as $post): ?>
        <?php $title = $post['title'] ?: 'Untitled'; ?>
        <div class="feed-item">
            <a href="<?= h(mediaUrl($db, 'uploads', $post['filename'])) ?>">
                <img src="<?= h(mediaUrl($db, 'thumbs', $post['thumbname'])) ?>" alt="<?= h($title) ?>" loading="lazy">
            </a>
            <div class="info">
                <div class="title"><?= h($title) ?></div>
                <div class="meta"><?= h($post['avatar_name'] ?: 'Unknown user') ?> · <?= h($post['grid_name'] ?: 'Unknown grid') ?></div>
                <div class="meta">Added <?= h(gmdate('Y-m-d H:i', strtotime($post['created_at']))) ?> UTC</div>
                <div class="meta">Visibility: <?= h($post['visibility']) ?></div>

                <p><a href="post.php?id=<?= rawurlencode($post['token']) ?>&amp;owner=<?= rawurlencode($ownerToken) ?>">Open post</a></p>

                <form method="post" action="owner.php">
                    <input type="hidden" name="token" value="<?= h($ownerToken) ?>">
                    <input type="hidden" name="post_id" value="<?= (int)$post['id'] ?>">
                    <input type="hidden" name="action" value="visibility">
                    <label>Visibility<br>
                        <select name="visibility">
                            <option value="public" <?= $post['visibility'] === VISIBILITY_PUBLIC ? 'selected' : '' ?>>Public</option>
                            <option value="unlisted" <?= $post['visibility'] === VISIBILITY_UNLISTED ? 'selected' : '' ?>>Unlisted</option>
                            <option value="private" <?= $post['visibility'] === VISIBILITY_PRIVATE ? 'selected' : '' ?>>Private</option>
                        </select>
                    </label>
                    <button type="submit" class="btn btn-small">Save</button>
                </form>

                <form method="post" action="owner.php" onsubmit="return confirm('Delete this upload permanently?')">
                    <input type="hidden" name="token" value="<?= h($ownerToken) ?>">
                    <input type="hidden" name="post_id" value="<?= (int)$post['id'] ?>">
                    <button type="submit" name="action" value="delete" class="btn btn-small">Delete</button>
                </form>
            </div>
        </div>
    <?php endforeach; ?>
    </div>
<?php endif; ?>

<?php
htmlFooter();

function ownerNotFound(): void
{
    http_response_code(404);
    htmlHeader('Not Found');
    echo '<div class="error">Owner link not found.</div>';
    htmlFooter();
    exit;
}

function loadOwnedPosts(PDO $db, array $ownerPost, string $ownerUserUuid): array
{
    if ($ownerUserUuid !== '') {
        $stmt = $db->prepare('SELECT * FROM posts WHERE user_uuid = :user_uuid AND hidden = 0 ORDER BY created_at DESC');
        $stmt->execute([':user_uuid' => $ownerUserUuid]);
        return $stmt->fetchAll(PDO::FETCH_ASSOC);
    }

    $stmt = $db->prepare('SELECT * FROM posts WHERE id = :id AND hidden = 0');
    $stmt->execute([':id' => $ownerPost['id']]);
    return $stmt->fetchAll(PDO::FETCH_ASSOC);
}

function findOwnedPost(PDO $db, int $postId, array $ownerPost, string $ownerUserUuid): ?array
{
    if ($postId <= 0) {
        $postId = (int)$ownerPost['id'];
    }

    if ($ownerUserUuid !== '') {
        $stmt = $db->prepare('SELECT * FROM posts WHERE id = :id AND user_uuid = :user_uuid AND hidden = 0');
        $stmt->execute([':id' => $postId, ':user_uuid' => $ownerUserUuid]);
    } else {
        $stmt = $db->prepare('SELECT * FROM posts WHERE id = :id AND owner_token = :owner_token AND hidden = 0');
        $stmt->execute([':id' => $postId, ':owner_token' => $ownerPost['owner_token']]);
    }

    $post = $stmt->fetch(PDO::FETCH_ASSOC);
    return $post ?: null;
}
