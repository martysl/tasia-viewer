<?php
declare(strict_types=1);

require_once __DIR__ . '/config.php';
require_once __DIR__ . '/db.php';

$ownerId = trim((string)($_GET['id'] ?? $_GET['uuid'] ?? ''));
if ($ownerId === '') {
    http_response_code(404);
    htmlHeader('Owner Not Found');
    echo '<div class="error">Missing owner ID.</div>';
    htmlFooter();
    exit;
}

$page = max(1, (int)($_GET['page'] ?? 1));
$offset = ($page - 1) * ITEMS_PER_PAGE;

$db = getDB();

$ownerStmt = $db->prepare('
    SELECT avatar_name, grid_name, user_uuid
    FROM posts
    WHERE user_uuid = :owner_id AND visibility = "public" AND hidden = 0
    ORDER BY created_at DESC
    LIMIT 1
');
$ownerStmt->execute([':owner_id' => $ownerId]);
$owner = $ownerStmt->fetch(PDO::FETCH_ASSOC);

if (!$owner) {
    http_response_code(404);
    htmlHeader('Owner Not Found');
    echo '<div class="error">Owner not found, or this owner has no public posts.</div>';
    htmlFooter();
    exit;
}

$countStmt = $db->prepare('SELECT COUNT(*) FROM posts WHERE user_uuid = :owner_id AND visibility = "public" AND hidden = 0');
$countStmt->execute([':owner_id' => $ownerId]);
$total = (int)$countStmt->fetchColumn();
$maxPage = max(1, (int)ceil($total / ITEMS_PER_PAGE));

$stmt = $db->prepare('
    SELECT token, thumbname, title, avatar_name, grid_name, region_name, created_at
    FROM posts
    WHERE user_uuid = :owner_id AND visibility = "public" AND hidden = 0
    ORDER BY created_at DESC
    LIMIT :limit OFFSET :offset
');
$stmt->bindValue(':owner_id', $ownerId, PDO::PARAM_STR);
$stmt->bindValue(':limit', ITEMS_PER_PAGE, PDO::PARAM_INT);
$stmt->bindValue(':offset', $offset, PDO::PARAM_INT);
$stmt->execute();
$posts = $stmt->fetchAll(PDO::FETCH_ASSOC);

$ownerName = $owner['avatar_name'] ?: 'Unknown';
htmlHeader($ownerName);
?>

<div class="post-meta">
    <h2><?= htmlspecialchars($ownerName) ?></h2>
    <?php if (!empty($owner['grid_name'])): ?><p>Grid: <?= htmlspecialchars($owner['grid_name']) ?></p><?php endif; ?>
    <p><?= $total ?> public snapshot<?= $total === 1 ? '' : 's' ?></p>
</div>

<?php if (empty($posts)): ?>
<div class="notice">No public snapshots for this owner.</div>
<?php else: ?>
<div class="feed">
<?php foreach ($posts as $post): ?>
    <div class="feed-item">
        <a href="post.php?id=<?= rawurlencode($post['token']) ?>">
            <img src="<?= THUMBS_URL . '/' . htmlspecialchars($post['thumbname']) ?>" alt="" loading="lazy">
        </a>
        <div class="info">
            <div class="title"><a href="post.php?id=<?= rawurlencode($post['token']) ?>"><?= htmlspecialchars($post['title'] ?: 'Untitled') ?></a></div>
            <?php if (!empty($post['region_name'])): ?><div class="meta"><?= htmlspecialchars($post['region_name']) ?></div><?php endif; ?>
            <div class="meta"><?= htmlspecialchars(gmdate('Y-m-d H:i', strtotime($post['created_at']))) ?></div>
        </div>
    </div>
<?php endforeach; ?>
</div>

<div class="pagination">
<?php if ($page > 1): ?>
    <a href="?id=<?= rawurlencode($ownerId) ?>&amp;page=<?= $page - 1 ?>">&laquo; Previous</a>
<?php endif; ?>
    <span>Page <?= $page ?> of <?= $maxPage ?></span>
<?php if ($page < $maxPage): ?>
    <a href="?id=<?= rawurlencode($ownerId) ?>&amp;page=<?= $page + 1 ?>">Next &raquo;</a>
<?php endif; ?>
</div>
<?php endif; ?>

<p><a href="<?= BASE_URL ?>/">&laquo; Back to feed</a></p>

<?php htmlFooter();
