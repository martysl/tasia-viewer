<?php
declare(strict_types=1);

require_once __DIR__ . '/config.php';
require_once __DIR__ . '/db.php';

$page = max(1, (int)($_GET['page'] ?? 1));
$mode = (string)($_GET['mode'] ?? 'folders');
if (!in_array($mode, ['folders', 'list'], true)) {
    $mode = 'folders';
}
$selectedGrid = mb_substr(trim((string)($_GET['grid'] ?? '')), 0, 255);
$selectedAvatar = mb_substr(trim((string)($_GET['avatar'] ?? '')), 0, 255);

$db = getDB();

htmlHeader('Feed');
?>

<div class="view-switch">
    <a class="btn btn-small" href="?mode=folders">Folder view</a>
    <a class="btn btn-small" href="?mode=list">List view</a>
</div>

<?php if ($mode === 'folders'): ?>
    <?php if ($selectedGrid === ''): ?>
        <div class="breadcrumb">Choose a grid folder:</div>
        <?php
        $stmt = $db->query('
            SELECT COALESCE(NULLIF(grid_name, ""), "Unknown grid") AS grid_name, COUNT(*) AS count
            FROM posts
            WHERE visibility = "public" AND hidden = 0
            GROUP BY COALESCE(NULLIF(grid_name, ""), "Unknown grid")
            ORDER BY grid_name COLLATE NOCASE
        ');
        $folders = $stmt->fetchAll(PDO::FETCH_ASSOC);
        ?>
        <?php if (empty($folders)): ?>
            <div class="notice">No snapshots yet. Be the first!</div>
        <?php else: ?>
            <div class="folder-grid">
                <?php foreach ($folders as $folder): ?>
                    <a class="folder-card" href="?mode=folders&amp;grid=<?= rawurlencode($folder['grid_name']) ?>">
                        <span class="icon">📁</span>
                        <span class="name"><?= h($folder['grid_name']) ?></span>
                        <span class="count"><?= (int)$folder['count'] ?> posts</span>
                    </a>
                <?php endforeach; ?>
            </div>
        <?php endif; ?>
    <?php elseif ($selectedAvatar === ''): ?>
        <div class="breadcrumb"><a href="?mode=folders">Grids</a> / <?= h($selectedGrid) ?></div>
        <?php
        $stmt = $db->prepare('
            SELECT COALESCE(NULLIF(avatar_name, ""), "Unknown user") AS avatar_name, COUNT(*) AS count
            FROM posts
            WHERE visibility = "public" AND hidden = 0 AND COALESCE(NULLIF(grid_name, ""), "Unknown grid") = :grid
            GROUP BY COALESCE(NULLIF(avatar_name, ""), "Unknown user")
            ORDER BY avatar_name COLLATE NOCASE
        ');
        $stmt->execute([':grid' => $selectedGrid]);
        $folders = $stmt->fetchAll(PDO::FETCH_ASSOC);
        ?>
        <div class="folder-grid">
            <?php foreach ($folders as $folder): ?>
                <a class="folder-card" href="?mode=folders&amp;grid=<?= rawurlencode($selectedGrid) ?>&amp;avatar=<?= rawurlencode($folder['avatar_name']) ?>">
                    <span class="icon">📂</span>
                    <span class="name"><?= h($folder['avatar_name']) ?></span>
                    <span class="count"><?= (int)$folder['count'] ?> posts</span>
                </a>
            <?php endforeach; ?>
        </div>
    <?php else: ?>
        <div class="breadcrumb"><a href="?mode=folders">Grids</a> / <a href="?mode=folders&amp;grid=<?= rawurlencode($selectedGrid) ?>"><?= h($selectedGrid) ?></a> / <?= h($selectedAvatar) ?></div>
        <?php renderPosts($db, $selectedGrid, $selectedAvatar, $page, true); ?>
    <?php endif; ?>
<?php else: ?>
    <?php renderFilters($db, $selectedGrid, $selectedAvatar); ?>
    <?php renderPosts($db, $selectedGrid, $selectedAvatar, $page, false); ?>
<?php endif; ?>

<?php
htmlFooter();

function renderFilters(PDO $db, string $selectedGrid, string $selectedAvatar): void
{
    $grids = $db->query('SELECT DISTINCT grid_name FROM posts WHERE visibility = "public" AND hidden = 0 AND grid_name != "" ORDER BY grid_name COLLATE NOCASE')->fetchAll(PDO::FETCH_COLUMN);
    $avatarWhere = 'visibility = "public" AND hidden = 0 AND avatar_name != ""';
    $avatarParams = [];
    if ($selectedGrid !== '') {
        $avatarWhere .= ' AND grid_name = :grid_name';
        $avatarParams[':grid_name'] = $selectedGrid;
    }
    $avatarStmt = $db->prepare('SELECT DISTINCT avatar_name FROM posts WHERE ' . $avatarWhere . ' ORDER BY avatar_name COLLATE NOCASE');
    $avatarStmt->execute($avatarParams);
    $avatars = $avatarStmt->fetchAll(PDO::FETCH_COLUMN);
    ?>
    <form method="get" class="filters">
        <input type="hidden" name="mode" value="list">
        <label>Grid<br>
            <select name="grid" onchange="this.form.submit()">
                <option value="">All grids</option>
                <?php foreach ($grids as $grid): ?>
                    <option value="<?= h($grid) ?>" <?= $grid === $selectedGrid ? 'selected' : '' ?>><?= h($grid) ?></option>
                <?php endforeach; ?>
            </select>
        </label>
        <label>User<br>
            <select name="avatar" onchange="this.form.submit()">
                <option value="">All users</option>
                <?php foreach ($avatars as $avatar): ?>
                    <option value="<?= h($avatar) ?>" <?= $avatar === $selectedAvatar ? 'selected' : '' ?>><?= h($avatar) ?></option>
                <?php endforeach; ?>
            </select>
        </label>
        <noscript><button type="submit" class="btn btn-small">Filter</button></noscript>
        <?php if ($selectedGrid !== '' || $selectedAvatar !== ''): ?>
            <a class="btn btn-small" href="?mode=list">Clear</a>
        <?php endif; ?>
    </form>
    <?php
}

function renderPosts(PDO $db, string $selectedGrid, string $selectedAvatar, int $page, bool $exactFolder): void
{
    $where = ['visibility = "public"', 'hidden = 0'];
    $params = [];
    if ($selectedGrid !== '') {
        $field = $exactFolder ? 'COALESCE(NULLIF(grid_name, ""), "Unknown grid")' : 'grid_name';
        $where[] = $field . ' = :grid_name';
        $params[':grid_name'] = $selectedGrid;
    }
    if ($selectedAvatar !== '') {
        $field = $exactFolder ? 'COALESCE(NULLIF(avatar_name, ""), "Unknown user")' : 'avatar_name';
        $where[] = $field . ' = :avatar_name';
        $params[':avatar_name'] = $selectedAvatar;
    }
    $whereSql = implode(' AND ', $where);

    $totalStmt = $db->prepare('SELECT COUNT(*) FROM posts WHERE ' . $whereSql);
    $totalStmt->execute($params);
    $total = (int)$totalStmt->fetchColumn();
    $maxPage = max(1, (int)ceil($total / ITEMS_PER_PAGE));
    $page = min($page, $maxPage);
    $offset = ($page - 1) * ITEMS_PER_PAGE;

    $stmt = $db->prepare('
        SELECT token, thumbname, title, avatar_name, grid_name, created_at
        FROM posts
        WHERE ' . $whereSql . '
        ORDER BY grid_name COLLATE NOCASE ASC, avatar_name COLLATE NOCASE ASC, created_at DESC
        LIMIT :limit OFFSET :offset
    ');
    foreach ($params as $key => $value) {
        $stmt->bindValue($key, $value, PDO::PARAM_STR);
    }
    $stmt->bindValue(':limit', ITEMS_PER_PAGE, PDO::PARAM_INT);
    $stmt->bindValue(':offset', $offset, PDO::PARAM_INT);
    $stmt->execute();
    $posts = $stmt->fetchAll(PDO::FETCH_ASSOC);

    if (empty($posts)) {
        echo '<div class="notice">No snapshots here yet.</div>';
        return;
    }

    $currentGroup = null;
    foreach ($posts as $post) {
        $group = ($post['grid_name'] ?: 'Unknown grid') . ' · ' . ($post['avatar_name'] ?: 'Unknown user');
        if ($group !== $currentGroup) {
            if ($currentGroup !== null) {
                echo '</div>';
            }
            $currentGroup = $group;
            echo '<h2 class="group-title">' . h($group) . '</h2><div class="feed">';
        }
        ?>
        <div class="feed-item">
            <a href="post.php?id=<?= rawurlencode($post['token']) ?>">
                <img src="<?= h(mediaUrl($db, 'thumbs', $post['thumbname'])) ?>" alt="" loading="lazy">
            </a>
            <div class="info">
                <div class="title"><a href="post.php?id=<?= rawurlencode($post['token']) ?>"><?= h($post['title'] ?: 'Untitled') ?></a></div>
                <div class="meta">Added <?= h(gmdate('Y-m-d H:i', strtotime($post['created_at']))) ?> UTC</div>
            </div>
        </div>
        <?php
    }
    if ($currentGroup !== null) {
        echo '</div>';
    }

    $queryBase = ['mode' => (string)($_GET['mode'] ?? 'folders'), 'grid' => $selectedGrid, 'avatar' => $selectedAvatar];
    ?>
    <div class="pagination">
    <?php if ($page > 1): ?>
        <a href="?<?= h(http_build_query($queryBase + ['page' => $page - 1])) ?>">&laquo; Previous</a>
    <?php endif; ?>
        <span>Page <?= $page ?> of <?= $maxPage ?></span>
    <?php if ($page < $maxPage): ?>
        <a href="?<?= h(http_build_query($queryBase + ['page' => $page + 1])) ?>">Next &raquo;</a>
    <?php endif; ?>
    </div>
    <?php
}
